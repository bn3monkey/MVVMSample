#ifndef __BN3MONKEY_MEMORY_POOL_IMPL__
#define __BN3MONKEY_MEMORY_POOL_IMPL__

#include <mutex>
#include <vector>
#include <functional>

#include "../Log/Log.hpp"

#ifdef BN3MONKEY_DEBUG
#define FOR_DEBUG(t) t
#else 
#define FOR_DEBUG(t)
#endif

#ifdef __BN3MONKEY_LOG__
#ifdef BN3MONKEY_DEBUG
#define LOG_D(text, ...) Bn3Monkey::Log::D(__FUNCTION__, text, __VA_ARGS__)
#else
#define LOG_D(text, ...)
#endif
#define LOG_V(text, ...) Bn3Monkey::Log::V(__FUNCTION__, text, __VA_ARGS__)
#define LOG_E(text, ...) Bn3Monkey::Log::E(__FUNCTION__, text, __VA_ARGS__)
#else
#define LOG_D(text, ...)    
#define LOG_V(text, ...) 
#define LOG_E(text, ...)
#endif

namespace Bn3Monkey
{
    constexpr size_t BLOCK_SIZE_POOL[] = { 32, 64, 128, 256, 512, 1024, 4096, 8192, 8192};
    constexpr size_t BLOCK_SIZE_POOL_LENGTH = sizeof(BLOCK_SIZE_POOL) / sizeof(size_t) - 1;
    constexpr size_t MAX_BLOCK_SIZE = BLOCK_SIZE_POOL[BLOCK_SIZE_POOL_LENGTH - 1];
    constexpr size_t HEADER_SIZE = 16;

    template<size_t BlockSize>
    struct Bn3MemoryBlock
    {
        constexpr static unsigned int MAGIC_NUMBER = 0xFEDCBA98;
        struct alignas(HEADER_SIZE) Bn3MemoryHeader
        {
            const unsigned int dirty = 0xFEDCBA98;
            bool is_allocated{ false };
            Bn3MemoryBlock<BlockSize>* freed_ptr{nullptr};
        };

        constexpr static size_t size = BlockSize;
        constexpr static size_t header_size = HEADER_SIZE;
        constexpr static size_t content_size = BlockSize - header_size;

        Bn3MemoryHeader header;
        char content[content_size]{ 0 };

        static Bn3MemoryBlock<BlockSize>* getBlockReference(void* ptr)
        {
            if ((void *)nullptr <= ptr && ptr < (void*)header_size)
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

    template<size_t ObjectSize>
    class Bn3BlockHelper
    {    
    private:    
        template<size_t ObjectSize_, size_t idx>
        class Indexer
        {
        public:
            constexpr static size_t find() {
                return ObjectSize_ <= Bn3MemoryBlock<BLOCK_SIZE_POOL[idx]>::content_size ?
                    idx // true
                    :
                    Indexer<ObjectSize_, idx + 1>::find();
            }

        };

        template<size_t ObjectSize_>
        class Indexer<ObjectSize_, BLOCK_SIZE_POOL_LENGTH>
        {
        public:
            constexpr static size_t find() {
                return BLOCK_SIZE_POOL_LENGTH;
            }
        };

    public:
        static size_t idxOfArray(size_t array_size)
        {
            size_t whole_size = array_size * ObjectSize;
            for (size_t idx = 0; idx < BLOCK_SIZE_POOL_LENGTH; idx++)
            {
                size_t content_size = BLOCK_SIZE_POOL[idx] - HEADER_SIZE;
                if (whole_size <= content_size)
                {
                    return idx;
                }
            }
            return BLOCK_SIZE_POOL_LENGTH;
        }

        static constexpr size_t idx = Indexer<ObjectSize, 0>::find();
        static constexpr size_t size = Bn3MemoryBlock<BLOCK_SIZE_POOL[idx]>::size;
        static constexpr size_t content_size = Bn3MemoryBlock<BLOCK_SIZE_POOL[idx]>::content_size;
    };

    template<size_t idx>
    class Bn3MemoryBlockPool : public Bn3MemoryBlockPool<idx-1>
    {
    public:
        template<typename Size, typename... Sizes>
        bool initialize(Size size, Sizes... sizes)
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

            return reinterpret_cast<Bn3MemoryBlockPool<idx-1>*>(this)->initialize(std::forward<size_t>(sizes)...);
        }

        void release()
        {
            LOG_D("Max allocated value is (%llu) in Memory Block Pool (%d)", max_allocated, idx);
            blocks.clear();
            freed_ptr = nullptr;
            front = nullptr;
            back = nullptr;

            reinterpret_cast<Bn3MemoryBlockPool<idx - 1>*>(this)->release();
        }

        void* allocate()
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
            }

            auto* ptr = ret->content;
            LOG_D("Memory block pool (idx : %d / block size : %d) allocates %d", idx, block_size, ret - front);
            return ptr;
        }

        bool deallocate(void* ptr)
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

            size_t max_allocated{0};
            size_t current_allocated{ 0 };
            std::mutex mutex;

    };

    template<>
    class Bn3MemoryBlockPool<0>
    {
    public:

        template<typename Size, typename... Sizes>
        bool initialize(Size size, Sizes... sizes)
        {
            LOG_D("Memory block pool (idx : %d / block size : %d) initialize with a size of %d", 0, block_size, size);
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
        void release()
        {
            LOG_D("Max allocated value is (%llu) in Memory Block Pool (%d)", max_allocated, 0);
            blocks.clear();
            freed_ptr = nullptr;
            front = nullptr;
            back = nullptr;
        }

        void* allocate()
        {
            Bn3MemoryBlock<block_size>* ret{ nullptr };
            {
                std::lock_guard<std::mutex> lock(mutex);


                if (freed_ptr == nullptr)
                {
                    LOG_E("The capacity of memory block pool (idx : %d / block size : %d) has been exceeded", 0, block_size);
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
            }

            auto* ptr = ret->content;
            LOG_D("Memory block pool (idx : %d / block size : %d) allocates %d", 0, block_size, ret - front);
            return ptr;
        }
        bool deallocate(void* ptr)
        {
            auto* block_ptr = Bn3MemoryBlock<block_size>::getBlockReference(ptr);
            if (block_ptr < front || back < block_ptr)
            {
                LOG_E("This reference (%p) is not from memory block pool (idx : %d / block size : %d)", ptr, 0, block_size);
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
                block_ptr->header.freed_ptr = freed_ptr;
                freed_ptr = block_ptr;

            }

            LOG_D("Memory block pool (idx : %d / block size : %d) deallocates %d", 0, block_size, block_ptr - front);
            return true;
        }

    private:

        constexpr static size_t block_size = BLOCK_SIZE_POOL[0];

        std::vector<Bn3MemoryBlock<block_size>> blocks;
        Bn3MemoryBlock<block_size>* freed_ptr;

        Bn3MemoryBlock<block_size>* front;
        Bn3MemoryBlock<block_size>* back;

        size_t max_allocated{ 0 };
        size_t current_allocated{ 0 };
        std::mutex mutex;
    };

    template<size_t pool_size>
    class Bn3MemoryBlockPools : public Bn3MemoryBlockPool<pool_size-1>
    {
    public:
        static_assert(pool_size <= BLOCK_SIZE_POOL_LENGTH, "Static Memory Block Pools Size is invalid");
        constexpr static size_t max_pool_num = pool_size - 1;

        Bn3MemoryBlockPools()
        {
            setFunction<pool_size - 1>();
        }


        template<size_t size>
        void setFunction()
        {
            setFunction<size - 1>();
            allocators[size] = [&]() {
                return reinterpret_cast<Bn3MemoryBlockPool<size>*>(this)->allocate();
            };
            deallocators[size] = [&](void* ptr) {
                return reinterpret_cast<Bn3MemoryBlockPool<size>*>(this)->deallocate(ptr);
            };
        }

        template<>
        void setFunction<0>() {
            allocators[0] = [&]() {
                return reinterpret_cast<Bn3MemoryBlockPool<0>*>(this)->allocate();
            };
            deallocators[0] = [&](void* ptr) {
                return reinterpret_cast<Bn3MemoryBlockPool<0>*>(this)->deallocate(ptr);
            };
        }


        template<typename... Sizes>
        bool initialize(Sizes... sizes)
        {
            LOG_D("Memory Block Pools Initialize");
            return reinterpret_cast<Bn3MemoryBlockPool<max_pool_num>*>(this)->initialize(std::forward<size_t>(sizes)...);
        }
        void release()
        {
            LOG_D("Memory Block Pools release");
            reinterpret_cast<Bn3MemoryBlockPool<max_pool_num>*>(this)->release();
        }

        template<class Type, class... Args>
        Type* allocate_and_initialize(Args... args)
        {
            constexpr size_t object_size = sizeof(Type);
            constexpr size_t idx = Bn3BlockHelper<object_size>::idx;
            Type* ret = nullptr;

            if (idx >= pool_size)
            {
                ret = new Type(std::forward<Args>(args)...);
                return ret;
            }
            auto* ptr = reinterpret_cast<Bn3MemoryBlockPool<idx>*>(this)->allocate();
            if (!ptr)
            {
                LOG_E("The capcatiy of memory block pool has been exceeded");
                return nullptr;
            }
            ret = new (ptr) Type(std::forward<Args>(args)...);
            return ret;
        }

        template<class Type>
        Type* allocate_and_initialize()
        {
            constexpr size_t object_size = sizeof(Type);
            constexpr size_t idx = Bn3BlockHelper<object_size>::idx;
            Type* ret = nullptr;

            if (idx >= pool_size)
            {
                ret = new Type();
                return ret;
            }
            auto* ptr = reinterpret_cast<Bn3MemoryBlockPool<idx>*>(this)->allocate();
            if (!ptr)
            {
                LOG_E("The capcatiy of memory block pool has been exceeded");
                return nullptr;
            }
            ret = new (ptr) Type();
            return ret;
        }


        template<class Type>
        bool deallocate(Type* reference)
        {
            constexpr size_t object_size = sizeof(Type);
            constexpr size_t idx = Bn3BlockHelper<object_size>::idx;
            if (idx >= pool_size)
            {
                delete reference;
                return true;
            }

            if (reference)
                reference->~Type();
            return reinterpret_cast<Bn3MemoryBlockPool<idx>*>(this)->deallocate(reference);
        }

        template<class Type>
        Type* allocate(size_t size)
        {
            if (size == 1)
                return allocate_and_initialize<Type>();

            constexpr size_t object_size = sizeof(Type);
            size_t idx = Bn3BlockHelper<object_size>::idxOfArray(size);
            if (idx >= pool_size)
            {
                return new Type[size];
            }

            auto* ptr = allocators[idx]();
            if (!ptr)
            {
                LOG_E("The capcatiy of memory block pool has been exceeded");
                return nullptr;
            }
            Type* ret = reinterpret_cast<Type*>(ptr);
            for (size_t i = 0; i < size; i++)
            {
                new (ret + i) Type();
            }
            return ret;
        }


        template<class Type>
        bool deallocate(Type* reference, size_t size)
        {
            if (size == 1)
                return deallocate(reference);

            constexpr size_t object_size = sizeof(Type);
            size_t idx = Bn3BlockHelper<object_size>::idxOfArray(size);
            if (idx >= pool_size)
            {
                delete[] reference;
                return true;
            }

            if (reference)
            {
                for (size_t i = 0; i < size; i++)
                {
                    auto* ptr = reference + i;
                    ptr->~Type();
                }
            }
            bool ret = deallocators[idx](reference);
            return ret;
        }

    private:
        std::function<void* ()> allocators[pool_size];
        std::function<bool(void*)> deallocators[pool_size];
    };
}

#endif