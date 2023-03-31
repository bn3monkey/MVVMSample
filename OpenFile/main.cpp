#include "ScopedTask.hpp"
#include "ScopedTaskScopeImpl.hpp"
#include <cstdint>

struct Request {
    Bn3Monkey::ScopedTaskScopeImpl* scope {nullptr};
    bool is_activated{false};
    bool* is_done{ nullptr };
    Request() {}
    Request(Bn3Monkey::ScopedTaskScopeImpl* scope, bool is_activated, bool* is_done) : scope(scope), is_activated(is_activated), is_done(is_done) {}

};

std::queue<Request> _requests;
bool _is_running;
std::mutex _request_mtx;
std::condition_variable _request_cv;

void manager()
{
    LOG_V("Manager starts");
    

    for (;;)
    {
        Request request;

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _request_cv.wait(lock, [&]() {
                return !_is_running || !_requests.empty();
                });
            if (!_is_running)
            {
                LOG_D("Clear All Requests");
                while (!_requests.empty())
                {
                    request = std::move(_requests.front());
                    _requests.pop();
                    if (request.is_activated)
                    {
                        LOG_D("Manager starts Worker (%s)", request.scope->name());
                        request.scope->start();
                    }
                    else {
                        LOG_D("Manager stops Worker (%s)", request.scope->name());
                        request.scope->stop();
                    }
                }
                break;
            }
            request = std::move(_requests.front());
            _requests.pop();
        }

        if (request.is_activated)
        {
            LOG_D("Manager starts Worker (%s)", request.scope->name());
            request.scope->start();
        }
        else {
            LOG_D("Manager stops Worker (%s)", request.scope->name());
            request.scope->stop();
        }

    }
    

    Bn3Monkey::ScopedTaskScopeImpl::clearScope();
}
bool start(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to start task scope (%s)", task_scope.name());
    
    bool is_done {false};

    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        if (!_is_running)
        {
            LOG_D("Request manager is stopped");
            return false;
        }

        _requests.emplace(&task_scope, true, &is_done);
    }

    _request_cv.notify_all();
    return true;
}
bool stop(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{

    LOG_V("Request manager to stop task scope (%s)", task_scope.name());
    bool is_done{ false };

    {
       {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _requests.emplace(&task_scope, false, &is_done);
        }


       if (!_is_running)
       {
           LOG_D("Request manager is stopped");
           return false;
       }
        _request_cv.notify_all();
    }
    return true;
}

void testScopedTaskScope()
{
    bool error[100]{ false };

    error[1] = false;


    using namespace Bn3Monkey;
    auto* main = ScopedTaskScopeImpl::getScope("main",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    auto* device = ScopedTaskScopeImpl::getScope("device",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    auto* ip = ScopedTaskScopeImpl::getScope("ip",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    _is_running = true;
    std::thread _thread = std::thread(manager);

    main->run("big task", []() {
        for (int count = 5; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nCount : %d\n\n", count);
        }
        });

    main->run("big task2", []() {
        for (int count = 3; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nSANS : %d\n\n", count);
        }
        });
    
    auto main1 = main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("\nSum : %d\n\n", sum);
        });
    main1.wait();
    
    printf("-------- Main 1 Relaunch! --------\n");

    main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("\nSum : %d\n\n", sum);
        });

    printf("\n-------- Main 1 Relaunch! --------\n\n");

    auto main11 = main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("\nSum : %d\n\n", sum);
        return sum;
        });

    auto main_ret = main11.wait();
    if (main_ret)
    {
        printf("\nmain_ret : %d\n\n", *main_ret);
    }

    auto main2 = main->call("main2", [&]() {
        printf("\nmain2\n\n");
        auto device2 = device->call("device2", [&]() {
            printf("\ndevice2\n\n");

            if (error[0])
            {
                printf("\nmaybe error\n\n");
                auto main3 = main->call("main3", [&]() {
                    return 4;
                    });
                auto* main3_result = main3.wait();
                return 3;
            }
            return 0;
        });
        auto* device2_ret = device2.wait();
        return 2;
    });
    auto ret2 = main2.wait();
    if (ret2)
    {
        printf("\nret2 : %d\n\n", *ret2);
    }

    auto main3 = main->call("main3", [&]() {
        printf("\nmain3\n\n");
        auto device3 = device->call("device3", [&]() {
            printf("\ndevice3\n\n");
            auto ip3 = ip->call("ip3", [&]() {
                printf("\nip3\n\n");
                printf("\nmaybe error!\n\n");
                if (error[1])
                {
                    auto main4 = main->call("main4", [&]() {
                        printf("\nmain4\n\n");
                        });
                    return 4;
                }
                return 0;
                });
            auto* ip3_result = ip3.wait();
            return 3;
            });
        auto* device3_ret = device3.wait();
        return 2;
        });
    auto ret3 = main3.wait();
    if (ret3)
    {
        printf("\nret3 : %d\n\n", *ret3);
    }


    using namespace std::chrono_literals;

    class SANS
    {
    public:
        SANS(int a)
        {
            this->a = a;
        }
        int a;
    };

    auto ip_start = ip->call("ip_start", [&]() {
        printf("\nip Start\n\n");
        return SANS(2);
        });

    std::this_thread::sleep_for(9.5s);
    auto* sans2 = ip_start.wait();
    if (sans2)
    {
        printf("\nSANS : %d\n\n", sans2->a);
    }

    std::this_thread::sleep_for(1s);
    auto ip_end = ip->call("ip_end", [&]() {
        printf("\nip End\n\n");
        return SANS(4);
        });
    auto* sans4 = ip_end.wait();
    if (sans4)
    {
        printf("\nSANS : %d\n\n", sans4->a);
    }

    _is_running = false;
    _request_cv.notify_all();
    _thread.join();
}
;

constexpr size_t BLOCK_SIZE_POOL[] = { 32, 64, 128, 256, 512, 1024, 4096, 8192, 8192};
constexpr size_t BLOCK_SIZE_POOL_LENGTH = sizeof(BLOCK_SIZE_POOL) / sizeof(size_t) - 1;
constexpr size_t MAX_BLOCK_SIZE = BLOCK_SIZE_POOL[BLOCK_SIZE_POOL_LENGTH - 1];
constexpr size_t HEADER_SIZE = 16;



template<size_t BlockSize>
struct StaticMemoryBlock
{
    constexpr static unsigned int MAGIC_NUMBER = 0xFEDCBA98;
    struct alignas(HEADER_SIZE) MemoryHeader
    {
        const unsigned int dirty = 0xFEDCBA98;
        bool is_allocated{ false };
        StaticMemoryBlock<BlockSize>* freed_ptr{nullptr};
    };

    constexpr static size_t size = BlockSize;
    constexpr static size_t header_size = HEADER_SIZE;
    constexpr static size_t content_size = BlockSize - header_size;
    
    MemoryHeader header;
    char content[content_size]{ 0 };

    static StaticMemoryBlock<BlockSize>* getBlockReference(void* ptr)
    {
        if ((void *)nullptr <= ptr && ptr < (void*)header_size)
        {
            LOG_E("This reference is nullptr");
            return nullptr;
        }

        auto* content_ptr = reinterpret_cast<char*>(ptr);
        auto* block_ptr = content_ptr - header_size;
        auto* block_casted_ptr = reinterpret_cast<StaticMemoryBlock<BlockSize>*>(block_ptr);
        if (block_casted_ptr->header.dirty != MAGIC_NUMBER)
        {
            LOG_E("This reference cannot be transformed by block");
            return nullptr;
        }
        return block_casted_ptr;
    }
};

template<size_t ObjectSize>
class StaticMemoryBlockHelper
{    
private:    
    template<size_t ObjectSize_, size_t idx>
    class Indexer
    {
    public:
        constexpr static size_t find() {
            return ObjectSize_ <= StaticMemoryBlock<BLOCK_SIZE_POOL[idx]>::content_size ?
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
    static constexpr size_t size = StaticMemoryBlock<BLOCK_SIZE_POOL[idx]>::size;
    static constexpr size_t content_size = StaticMemoryBlock<BLOCK_SIZE_POOL[idx]>::content_size;

};

template<size_t idx>
class StaticMemoryBlockPool : public StaticMemoryBlockPool<idx - 1> {

public:

    template<typename Size, typename... Sizes>
    bool initialize(Size size, Sizes... sizes)
    {
        // static_assert(std::is_same<Size, size_t>::value, "Block Pool Size Type must be size_t");
        blocks.resize(size);
        
        front = &blocks.front();
        back = &blocks.back();

        freed_ptr = front;
        for (auto* block_ptr = front; block_ptr < back; block_ptr += 1)
        {
            block_ptr->header.freed_ptr = block_ptr + 1;
        }
        back->header.freed_ptr = nullptr;

        return reinterpret_cast<StaticMemoryBlockPool<idx-1>*>(this)->initialize(std::forward<size_t>(sizes)...);
    }

    void release()
    {
        LOG_D("%s : max_allocated (%llu)", __FUNCTION__, max_allocated);
        blocks.clear();
        freed_ptr = nullptr;
        front = nullptr;
        back = nullptr;

        reinterpret_cast<StaticMemoryBlockPool<idx - 1>*>(this)->release();
    }

    void* allocate()
    {
        StaticMemoryBlock<block_size>* ret{ nullptr };
        {
            std::lock_guard<std::mutex> lock(mutex);
                       

            if (freed_ptr == nullptr)
            {
                LOG_E("Static memory block pool cannot allocate");
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
        return ret->content;
    }
    bool deallocate(void* ptr)
    {
        auto* block_ptr = StaticMemoryBlock<block_size>::getBlockReference(ptr);
        if (block_ptr < front || back < block_ptr)
        {
            LOG_E("This reference is not from static memory block pool");
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (block_ptr->header.is_allocated == false)
            {
                LOG_E("This reference is already freed");
                return false;
            }

            current_allocated -= 1;

            block_ptr->header.is_allocated = false;
            block_ptr->header.freed_ptr = freed_ptr;
            freed_ptr = block_ptr;

        }
        return true;
    }

private:
    constexpr static size_t block_size = BLOCK_SIZE_POOL[idx];

    std::vector<StaticMemoryBlock<block_size>> blocks; 
    StaticMemoryBlock<block_size>* freed_ptr;

    StaticMemoryBlock<block_size>* front;
    StaticMemoryBlock<block_size>* back;

    size_t max_allocated{0};
    size_t current_allocated{ 0 };
    std::mutex mutex;

};


template<>
class StaticMemoryBlockPool<0>
{
public:

    template<typename Size, typename... Sizes>
    bool initialize(Size size, Sizes... sizes)
    {
        // static_assert(std::is_same<Size, size_t>::value, "Block Pool Size Type must be size_t");
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
        LOG_D("%s : max_allocated (%llu)\n", __FUNCTION__, max_allocated);
        blocks.clear();
        freed_ptr = nullptr;
        front = nullptr;
        back = nullptr;
    }

    void* allocate()
    {
        StaticMemoryBlock<block_size>* ret{ nullptr };
        {
            std::lock_guard<std::mutex> lock(mutex);


            if (freed_ptr == nullptr)
            {
                LOG_E("Static memory block pool cannot allocate");
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
        return ret->content;
    }
    bool deallocate(void* ptr)
    {
        auto* block_ptr = StaticMemoryBlock<block_size>::getBlockReference(ptr);
        if (block_ptr < front || back < block_ptr)
        {
            LOG_E("This reference is not from static memory block pool");
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (block_ptr->header.is_allocated == false)
            {
                LOG_E("This reference is already freed");
                return false;
            }

            current_allocated -= 1;

            block_ptr->header.is_allocated = false;
            block_ptr->header.freed_ptr = freed_ptr;
            freed_ptr = block_ptr;

        }
        return true;
    }

private:

    constexpr static size_t block_size = BLOCK_SIZE_POOL[0];

    std::vector<StaticMemoryBlock<block_size>> blocks;
    StaticMemoryBlock<block_size>* freed_ptr;

    StaticMemoryBlock<block_size>* front;
    StaticMemoryBlock<block_size>* back;

    size_t max_allocated{ 0 };
    size_t current_allocated{ 0 };
    std::mutex mutex;
};

template<size_t pool_size>
class StaticMemoryBlockPools : public StaticMemoryBlockPool<pool_size-1>
{
public:
    static_assert(pool_size <= BLOCK_SIZE_POOL_LENGTH, "Static Memory Block Pools Size is invalid");
    constexpr static size_t max_pool_num = pool_size - 1;

    StaticMemoryBlockPools()
    {
        setFunction<pool_size - 1>();
    }


    template<size_t size>
    void setFunction()
    {
        setFunction<size - 1>();
        allocators[size] = [&]() {
            return reinterpret_cast<StaticMemoryBlockPool<size>*>(this)->allocate();
        };
        deallocators[size] = [&](void* ptr) {
            return reinterpret_cast<StaticMemoryBlockPool<size>*>(this)->deallocate(ptr);
        };
    }
    template<>
    void setFunction<0>() {
        allocators[0] = [&]() {
            return reinterpret_cast<StaticMemoryBlockPool<0>*>(this)->allocate();
        };
        deallocators[0] = [&](void* ptr) {
            return reinterpret_cast<StaticMemoryBlockPool<0>*>(this)->deallocate(ptr);
        };
    }


    template<typename... Sizes>
    bool initialize(Sizes... sizes)
    {
        return reinterpret_cast<StaticMemoryBlockPool<max_pool_num>*>(this)->initialize(std::forward<size_t>(sizes)...);
    }
    void release()
    {
        reinterpret_cast<StaticMemoryBlockPool<max_pool_num>*>(this)->release();
    }

    template<class Type>
    Type* allocate()
    {
        constexpr size_t object_size = sizeof(Type);
        constexpr size_t idx = StaticMemoryBlockHelper<object_size>::idx;
        if (idx >= pool_size)
            return new Type();
        auto* ret = reinterpret_cast<StaticMemoryBlockPool<idx>*>(this)->allocate();
        return reinterpret_cast<Type*>(ret);
    }

    template<class Type>
    bool deallocate(Type* reference)
    {
        constexpr size_t object_size = sizeof(Type);
        constexpr size_t idx = StaticMemoryBlockHelper<object_size>::idx;
        if (idx >= pool_size)
        {
            delete reference;
            return true;
        }
        return reinterpret_cast<StaticMemoryBlockPool<idx>*>(this)->deallocate(reference);
    }

    template<class Type>
    Type* allocate(size_t size)
    {
        if (size == 1)
            return allocate<Type>();
        constexpr size_t object_size = sizeof(Type);
        size_t idx = StaticMemoryBlockHelper<object_size>::idxOfArray(size);
        if (idx >= pool_size)
        {
            return new Type[size];
        }
        auto* ret = allocators[idx]();
        return reinterpret_cast<Type*>(ret);
    }

    template<class Type>
    bool deallocate(Type* reference, size_t size)
    {
        if (size == 1)
            return deallocate(reference);
        constexpr size_t object_size = sizeof(Type);
        size_t idx = StaticMemoryBlockHelper<object_size>::idxOfArray(size);
        if (idx >= pool_size)
        {
            delete[] reference;
            return true;
        }
        bool ret = deallocators[idx](reference);
        return ret;
    }
private:
    std::function<void* ()> allocators[pool_size];
    std::function<bool(void*)> deallocators[pool_size];
};


void main()
{
    // testScopedTaskScope();

    StaticMemoryBlockPools<8> pools;
    pools.initialize(16, 16, 16, 16, 16, 16, 16, 16);

    auto* a = pools.allocate<int>(1);
    pools.deallocate(a, 1);

    auto* a2 = pools.allocate<int>(3);
    pools.deallocate(a, 3);

    auto* b = pools.allocate<int>(255);
    pools.deallocate(b, 255);

    class SANS
    {
        char tt[9000];
    };

    auto* d = pools.allocate<SANS>();
    pools.deallocate(d);

    auto* e = pools.allocate<SANS>(3);
    pools.deallocate<SANS>(e, 3);



    auto* c = pools.allocate<std::function<void(int, int ,int)>>(3);

    c[0] = [&](int a, int b, int c) {
        printf("%d\n", a + b + c);
    };
    c[1] = [&](int a, int b, int c) {
        printf("%d\n", 2*a + b + c);
    };
    c[2] = [&](int a, int b, int c) {
        printf("%d\n", a + 3*b + c);
    };

    c[0](3, 5, 2);
    c[1](2, 3, 4);
    c[2](5, 1, 2);

    pools.deallocate(c, 3);
    

    pools.release();

    /*
    StaticMemoryBlock<32> block;
    StaticMemoryBlock<64> block2;
    StaticMemoryBlock<128> block3;

    size_t size = StaticMemoryBlock<32>::size;
    size_t size2 = StaticMemoryBlock<64>::size;
    size_t size3 = StaticMemoryBlock<128>::size;

    printf("%d %d %d\n", size, size2, size3);

    size_t content_size = StaticMemoryBlock<BLOCK_SIZE_POOL[0]>::size;
    size_t content_size2 = StaticMemoryBlock<BLOCK_SIZE_POOL[1]>::size;

    printf("%d %d\n", content_size, content_size2);


    {
        size_t idx1 = StaticMemoryBlockHelper<10>::idx;
        size_t idx2 = StaticMemoryBlockHelper<50>::idx;
        size_t idx3 = StaticMemoryBlockHelper<70>::idx;

        {
            size_t a = StaticMemoryBlockHelper<10>::size;
            size_t b = StaticMemoryBlockHelper<50>::size;
            size_t c = StaticMemoryBlockHelper<70>::size;
        }

        // size_t idx1 = getIdx<28>();
        // size_t idx2 = getIdx<43>();
        // size_t idx3 = getIdx<125>();

        printf("%d %d %d\n", idx1, idx2, idx3);
    }
    
    

    if (true)
    {
        StaticMemoryBlockPool<0> blockpool;
        blockpool.initialize(4);

        auto* ret1 = blockpool.allocate();
        auto* ret2 = blockpool.allocate();
        auto* ret3 = blockpool.allocate();
        auto* ret4 = blockpool.allocate();
        auto* ret5 = blockpool.allocate();

        blockpool.deallocate(ret4);
        blockpool.deallocate(ret1);
        blockpool.deallocate(ret3);
        blockpool.deallocate(ret2);

        ret5 = blockpool.allocate();
        blockpool.deallocate(ret5);

        blockpool.release();
    }

    if (true)
    {
        StaticMemoryBlockPool<0> blockpool;
        blockpool.initialize(4);

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            blockpool.deallocate(ret2);
            blockpool.deallocate(ret1);
        }

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            auto* ret3 = blockpool.allocate();
            blockpool.deallocate(ret2);
            blockpool.deallocate(ret1);
        }

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            auto* ret3 = blockpool.allocate();
            auto* ret4 = blockpool.allocate();
            blockpool.deallocate(ret3);
            blockpool.deallocate(ret1);
            blockpool.deallocate(ret2);
        }

        blockpool.release();
    }

    if (true)
    {
        StaticMemoryBlockPool<1> blockpool;
        blockpool.initialize(4, 4);

        auto* ret1 = blockpool.allocate();
        auto* ret2 = blockpool.allocate();
        auto* ret3 = blockpool.allocate();
        auto* ret4 = blockpool.allocate();
        auto* ret5 = blockpool.allocate();

        blockpool.deallocate(ret4);
        blockpool.deallocate(ret1);
        blockpool.deallocate(ret3);
        blockpool.deallocate(ret2);

        ret5 = blockpool.allocate();
        blockpool.deallocate(ret5);

        blockpool.release();
    }

    if (true)
    {
        StaticMemoryBlockPool<1> blockpool;
        blockpool.initialize(4, 4);

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            blockpool.deallocate(ret2);
            blockpool.deallocate(ret1);
        }

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            auto* ret3 = blockpool.allocate();
            blockpool.deallocate(ret2);
            blockpool.deallocate(ret1);
        }

        {
            auto* ret1 = blockpool.allocate();
            auto* ret2 = blockpool.allocate();
            auto* ret3 = blockpool.allocate();
            auto* ret4 = blockpool.allocate();
            blockpool.deallocate(ret3);
            blockpool.deallocate(ret1);
            blockpool.deallocate(ret2);
        }

        blockpool.release();
    }

    if (true)
    {
        StaticMemoryBlockPool<1> blockpool;
        blockpool.initialize(5, 5);

        auto* ret1 = blockpool.allocate();
        blockpool.deallocate(ret1);
        blockpool.deallocate(ret1);

        ret1 = blockpool.allocate();
        auto* ret2 = blockpool.allocate();
        auto* ret3 = blockpool.allocate();
        auto* ret4 = blockpool.allocate();

        blockpool.deallocate(ret3);
        blockpool.deallocate(ret3);

        auto* sans = new int();
        blockpool.deallocate(sans);

        blockpool.deallocate(nullptr);

        blockpool.deallocate((void *)4);
    }
    */



    // StaticMemoryBlockPools<8> memorypools{};
    // memorypools.initialize(512u, 512u, 512u, 512u, 512u, 512u, 512u, 512u);
    printf("sans\n");

    // MemoryBlockHelper<28> helper;
    // MemoryBlockHelper<52> helper;

    return;
}
