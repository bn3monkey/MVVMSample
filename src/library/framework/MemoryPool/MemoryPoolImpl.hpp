#ifndef __BN3MONKEY_MEMORY_POOL_IMPL__
#define __BN3MONKEY_MEMORY_POOL_IMPL__

#include <mutex>
#include <vector>
#include <functional>

#include <string>
#include <sstream>
#include <cstdint>

#include "../Tag/Tag.hpp"
#include "../Log/Log.hpp"

#ifdef BN3MONKEY_DEBUG
#define FOR_DEBUG(t) t
#else 
#define FOR_DEBUG(t)
#endif

#ifdef __BN3MONKEY_LOG__
#ifdef BN3MONKEY_DEBUG
#define LOG_D(text, ...) Bn3Monkey::Log::D(__FUNCTION__, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)
#endif
#define LOG_V(text, ...) Bn3Monkey::Log::V(__FUNCTION__, text, ##__VA_ARGS__)
#define LOG_E(text, ...) Bn3Monkey::Log::E(__FUNCTION__, text, ##__VA_ARGS__)
#else
#define LOG_D(text, ...)    
#define LOG_V(text, ...) 
#define LOG_E(text, ...)
#endif

namespace Bn3Monkey
{

    constexpr size_t BLOCK_SIZE_POOL[] = { 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 0 };
    constexpr size_t BLOCK_SIZE_POOL_LENGTH = sizeof(BLOCK_SIZE_POOL) / sizeof(size_t) - 1;
    constexpr size_t MAX_BLOCK_SIZE = BLOCK_SIZE_POOL[BLOCK_SIZE_POOL_LENGTH - 1];
    constexpr size_t HEADER_SIZE = sizeof(unsigned int) + sizeof(int) + sizeof(void*) + sizeof(Bn3Tag);

    template<size_t BlockSize>
    struct Bn3MemoryBlock
    {
        constexpr static unsigned int MAGIC_NUMBER = 0xFEDCBA98;
        struct Bn3MemoryHeader
        {
            const unsigned int dirty = 0xFEDCBA98;
            Bn3Tag tag;
            int is_allocated{ false };
            Bn3MemoryBlock<BlockSize>* freed_ptr{ nullptr };
        };

        static_assert(HEADER_SIZE == sizeof(Bn3MemoryHeader));

        constexpr static size_t size = BlockSize;
        constexpr static size_t header_size = sizeof(Bn3MemoryHeader);
        constexpr static size_t content_size = BlockSize - header_size;

        Bn3MemoryHeader header;
        char content[content_size]{ 0 };

        static Bn3MemoryBlock<BlockSize>* getBlockReference(void* ptr)
        {
            if ((void*)nullptr <= ptr && ptr < (void*)header_size)
            {
                LOG_E("reference is nullptr (%p)", ptr);
                return nullptr;
            }

            auto* content_ptr = reinterpret_cast<char*>(ptr);
            auto* block_ptr = content_ptr - header_size;
            auto* block_casted_ptr = reinterpret_cast<Bn3MemoryBlock<BlockSize>*>(block_ptr);
            if (block_casted_ptr->header.dirty != MAGIC_NUMBER)
            {
                LOG_E("Reference (%p) cannot be transformed as memory block.", block_casted_ptr);
                LOG_E("Reference is not allocated by memory pool or memory header is corrupted");
                return nullptr;
            }
            return block_casted_ptr;
        }
    };

    template<size_t idx>
    constexpr size_t findBlockPoolIndex(size_t object_size)
    {
        return object_size <= Bn3MemoryBlock< BLOCK_SIZE_POOL[idx]>::content_size ?
            idx :
            findBlockPoolIndex<idx + 1>(object_size);
    }

    template<>
    constexpr size_t findBlockPoolIndex<BLOCK_SIZE_POOL_LENGTH>(size_t object_size)
    {
        return BLOCK_SIZE_POOL_LENGTH;
    }

    class Bn3BlockHelper
    {
    public:
        Bn3BlockHelper(size_t object_size)
        {
            _idx = findBlockPoolIndex<0>(object_size);
            _size = BLOCK_SIZE_POOL[_idx];
            _content_size = _size - HEADER_SIZE;
        }

        inline size_t idx() { return _idx; }
        inline size_t size() { return _size; }
        inline size_t content_size() { return _content_size; }

    private:
        size_t _idx;
        size_t _size;
        size_t _content_size;
    };


    class Bn3MemoryBlockPool
    {
    public:
        virtual bool initialize(size_t size) = 0;
        virtual void release() = 0;
        virtual std::string analyze() = 0;

        virtual void* allocate(const Bn3Tag& tag) = 0;
        virtual bool deallocate(void* ptr) = 0;

    protected:
        size_t max_allocated{ 0 };
        size_t current_allocated{ 0 };
        std::mutex mutex;
    };

    template<size_t idx>
    class Bn3MemoryBlockIndexedPool : public Bn3MemoryBlockPool
    {
    public:
        bool initialize(size_t size) override
        {
            LOG_D("Memory block pool (idx : %d / block size : %d) initialize with a size of %d", idx, block_size, size);
            blocks.resize(size);

            front = &blocks.front();
            back = &blocks.back();

            freed_ptr = front;
            for (auto* block_ptr = front; block_ptr < back; block_ptr += 1)
            {
                block_ptr->header.freed_ptr = block_ptr + 1;
            }
            back->header.freed_ptr = nullptr;

            return true;
        }

        void release() override {
            blocks.clear();
            freed_ptr = nullptr;
            front = nullptr;
            back = nullptr;
        }

        std::string analyze() override
        {
            std::stringstream ss;
            ss << "- Memory Block Pool (" << idx << " / " << block_size << ") - \n";
            ss << "    Max allocated : " << max_allocated << "\n\n";

            size_t start_idx = freed_ptr - front;

            for (size_t i = 0; i < blocks.size(); i++)
            {
                auto& block = blocks[i];
                auto& is_allocated = block.header.is_allocated;
                auto tag = block.header.tag.str();

                if (start_idx == i)
                    ss << "S";
                else
                    ss << " ";
                ss << "   (" << i << ") : " << (is_allocated ? "O" : "X") << " [" << tag << "] ";

                auto freed_ptr = block.header.freed_ptr;
                if (freed_ptr == nullptr)
                    ss << " -> null\n";
                else
                {
                    auto next_i = freed_ptr - front;
                    ss << " -> " << next_i << "\n";
                }
            }
            ss << "\n";
            return ss.str();
        }

        void* allocate(const Bn3Tag& tag) override
        {
            Bn3MemoryBlock<block_size>* ret{ nullptr };
            {
                std::lock_guard<std::mutex> lock(mutex);


                if (freed_ptr == nullptr)
                {
                    LOG_E("The capacity of memory block pool (idx : %d / block size : %d) has been exceeded", idx, block_size);
                    return nullptr;
                }

                current_allocated += 1;
                if (current_allocated > max_allocated)
                    max_allocated = current_allocated;


                ret = freed_ptr;
                auto* next_freed_ptr = freed_ptr->header.freed_ptr;
                freed_ptr->header.freed_ptr = nullptr;
                freed_ptr = next_freed_ptr;

                ret->header.is_allocated = true;
                ret->header.tag = tag;
            }

            auto* ptr = ret->content;
            LOG_D("Memory block pool (idx : %d / block size : %d) allocates %d", idx, block_size, ret - front);
            return ptr;
        }

        bool deallocate(void* ptr) override
        {
            auto* block_ptr = Bn3MemoryBlock<block_size>::getBlockReference(ptr);
            if (block_ptr < front || back < block_ptr)
            {
                LOG_E("This reference (%p) is not from memory block pool (idx : %d / block size : %d)", ptr, idx, block_size);
                return false;
            }
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (block_ptr->header.is_allocated == false)
                {
                    LOG_E("This reference (%d) is already deallocated", block_ptr - front);
                    return false;
                }

                current_allocated -= 1;

                block_ptr->header.is_allocated = false;
                block_ptr->header.tag.clear();

                block_ptr->header.freed_ptr = freed_ptr;
                freed_ptr = block_ptr;

            }

            LOG_D("Memory block pool (idx : %d / block size : %d) deallocates %d", idx, block_size, block_ptr - front);
            return true;
        }

    private:
        constexpr static size_t block_size = BLOCK_SIZE_POOL[idx];

        std::vector<Bn3MemoryBlock<block_size>> blocks;
        Bn3MemoryBlock<block_size>* freed_ptr;

        Bn3MemoryBlock<block_size>* front;
        Bn3MemoryBlock<block_size>* back;
    };

    template<size_t idx>
    constexpr size_t getStorageSize()
    {
        static_assert(1 < idx && idx <= BLOCK_SIZE_POOL_LENGTH);

        return sizeof(Bn3MemoryBlockIndexedPool<idx - 1>) + getStorageSize<idx - 1>();
    }

    template<>
    constexpr size_t getStorageSize<1>()
    {
        return sizeof(Bn3MemoryBlockIndexedPool<0>);
    }

    template<size_t pool_length, size_t idx>
    class Bn3BlockPoolInitializer
    {
    public:
        constexpr static void initialize(Bn3MemoryBlockPool* (&pool)[pool_length], char* storage, size_t offset)
        {

            using IndexedPool = Bn3MemoryBlockIndexedPool<idx>;
            size_t pool_size = sizeof(IndexedPool);
            auto* ptr = new (storage + offset - pool_size) IndexedPool();
            pool[idx] = ptr;

            Bn3BlockPoolInitializer<pool_length, idx - 1>::initialize(pool, storage, offset - pool_size);
        }
    };

    template<size_t pool_length>
    class Bn3BlockPoolInitializer<pool_length, 0>
    {
    public:
        constexpr static void initialize(Bn3MemoryBlockPool* (&pool)[pool_length], char* storage, size_t offset)
        {
            using IndexedPool = Bn3MemoryBlockIndexedPool<0>;
            size_t pool_size = sizeof(IndexedPool);
            assert(offset - pool_size == 0);
            auto* ptr = new (storage + offset - pool_size) IndexedPool();
            pool[0] = ptr;
        }
    };

    template<size_t pool_length>
    class Bn3MemoryBlockPools
    {
    public:
        static_assert(pool_length <= BLOCK_SIZE_POOL_LENGTH, "Static Memory Block Pools Size is invalid");
        constexpr static size_t max_pool_num = pool_length - 1;

        Bn3MemoryBlockPools()
        {
            Bn3BlockPoolInitializer<pool_length, max_pool_num>::initialize(_pools, _pool_storage, sizeof(_pool_storage));
        }

        bool initialize(std::initializer_list<size_t> sizes)
        {
            LOG_D("Memory Block Pools Initialize");
            size_t idx = 0;
            for (auto& size : sizes)
            {
                if (!_pools[idx++]->initialize(size))
                    return false;
            }
            return true;
        }
        void release()
        {
            LOG_D("Memory Block Pools release");
            for (auto* pool : _pools)
            {
                pool->release();
            }
        }

        template<class Type, class... Args>
        Type* construct(const Bn3Tag& tag, Args... args)
        {
            constexpr size_t object_size = sizeof(Type);
            size_t idx = Bn3BlockHelper(object_size).idx();
            Type* ret = nullptr;

            if (idx >= pool_length)
            {
                ret = new Type(std::forward<Args>(args)...);
                return ret;
            }
            auto* ptr = _pools[idx]->allocate(tag);
            if (!ptr)
            {
                LOG_E("The capcatiy of memory block pool has been exceeded");
                return nullptr;
            }
            ret = new (ptr) Type(std::forward<Args>(args)...);
            return ret;
        }

        template<class Type>
        bool destroy(Type* reference)
        {
            constexpr size_t object_size = sizeof(Type);
            size_t idx = Bn3BlockHelper(object_size).idx();
            if (idx >= pool_length)
            {
                delete reference;
                return true;
            }

            if (reference)
                reference->~Type();
            return _pools[idx]->deallocate(reference);
        }

        template<class Type>
        Type* allocate(const Bn3Tag& tag, size_t size)
        {
            constexpr size_t object_size = sizeof(Type);
            size_t allocated_size = sizeof(Type) * size;
            size_t idx = Bn3BlockHelper(allocated_size).idx();
            if (idx >= pool_length)
            {
                auto* ptr = new char[allocated_size];
                Type* ret = reinterpret_cast<Type*>(ptr);
                return ret;
            }

            auto* ptr = _pools[idx]->allocate(tag);
            if (!ptr)
            {
                LOG_E("The capcatiy of memory block pool has been exceeded");
                return nullptr;
            }
            Type* ret = reinterpret_cast<Type*>(ptr);
            return ret;
        }


        template<class Type>
        bool deallocate(Type* reference, size_t size)
        {
            constexpr size_t object_size = sizeof(Type);
            size_t allocated_size = sizeof(Type) * size;
            size_t idx = Bn3BlockHelper(allocated_size).idx();
            if (idx >= pool_length)
            {
                delete[](char*)reference;
                return true;
            }

            bool ret = _pools[idx]->deallocate(reference);
            return ret;
        }


        std::string analyzeAll()
        {
            std::stringstream ss;
            for (auto* pool : _pools)
            {
                ss << pool->analyze();
            }
            return ss.str();
        }

        std::string analyzePool(size_t idx)
        {
            return _pools[idx]->analyze();
        }

    private:

        Bn3MemoryBlockPool* _pools[pool_length];
        //char _pool_storage[8192];
        char _pool_storage[getStorageSize<pool_length>()];
    };
}

#endif