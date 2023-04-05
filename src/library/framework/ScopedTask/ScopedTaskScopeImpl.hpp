#ifndef __BN3MONKEY_TASK_SCOPE_IMPL__
#define __BN3MONKEY_TASK_SCOPE_IMPL__

#include "ScopedTaskImpl.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <cassert>


#ifdef BN3MONKEY_DEBUG
#define FOR_DEBUG(t) t
#else 
#define FOR_DEBUG(t)
#endif

#include "../Log/Log.hpp"

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

#include "../MemoryPool/MemoryPool.hpp"

#ifdef __BN3MONKEY_MEMORY_POOL__
#define MAKE_SHARED(TYPE, TAG, ...) Bn3Monkey::makeSharedFromMemoryPool<TYPE>(TAG, __VA_ARGS__)
#define Bn3Queue(TYPE) Bn3Monkey::Bn3Container::queue<TYPE>
#define Bn3Map(KEY, VALUE) Bn3Monkey::Bn3Container::map<KEY, VALUE>
#define Bn3String() Bn3Monkey::Bn3Container::string
#define Bn3Vector(TYPE) Bn3Monkey::Bn3Container::vector<TYPE>
#define Bn3Deque(TYPE) Bn3Monkey::Bn3Container::deque<TYPE>

#define Bn3QueueAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3MapAllocator(KEY, VALUE, TAG) Bn3Monkey::Bn3Allocator<std::pair<const KEY, VALUE>>(TAG)
#define Bn3StringAllocator(TAG) Bn3Monkey::Bn3Allocator<char>(TAG)
#define Bn3VectorAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3DequeAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)

#else
#define MAKE_SHARED(TYPE, TAG, ...) std::shared_ptr<TYPE>(new TYPE(__VA_ARGS__))
#define Bn3Queue(TYPE, TAG) std::queue<TYPE>
#define Bn3Map(KEY, VALUE, TAG) std::unordered_map<KEY, VALUE>
#define Bn3String(TAG) std::string
#define Bn3Vector(TYPE, TAG) std::vector<TYPE>
#define Bn3Deque(TYPE) std::deque<TYPE>

#define Bn3QueueAllocator(TYPE, TAG) 
#define Bn3MapAllocator(KEY, VALUE, TAG) 
#define Bn3StringAllocator(TAG) 
#define Bn3VectorAllocator(TYPE, TAG) 
#define Bn3DequeAllocator(TYPE, TAG)
#endif

namespace Bn3Monkey
{
    enum class ScopeState
    {
        IDLE = 0, // 할당된 작업 없으면 쓰레드도 동작하지 않음
        EMPTY = 2, // 할당된 작업 없음
        RUNNING = 3, // 할당된 작업 있음
    };

    class ScopedTaskScopeImplPool;

    class ScopedTaskScopeImpl
    {
    public:
        friend class ScopedTaskScopeImplPool;

        ScopedTaskScopeImpl(const Bn3Tag& scope_name,
            std::function<bool(ScopedTaskScopeImpl&)> onStart,
            std::function<ScopedTaskScopeImpl*()> getCurrentScope);

        ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other);

        virtual ~ScopedTaskScopeImpl();
       

        void start();
        void stop();

        template<class Func, class... Args>
        void run(const Bn3Tag& task_name, Func&& func, Args&&... args)
        {
            // Scope가 꺼져있으면 스코프 키기
            // 자기 자신 Scope에서 불렀으면 무조건 ScopeState는 Running
            // 다른 Scope에서 불렀으면 IDLE, EMPTY, READY, RUNNING 중 하나

            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            LOG_D("Make task (%s)", task_name.str());

            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (startScope(task))      
                    pushToScope(std::move(task));
            }

            LOG_D("Run Task (%s)", task_name.str());
            _cv.notify_all();
        }

        template<class Func, class... Args>
        auto call(const Bn3Tag& task_name, Func&& func, Args&&... args) -> std::shared_ptr<ScopedTaskResultImpl<std::result_of_t<Func(Args...)>>>
        {
            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            auto ret = task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            LOG_D("Make task (%s)", task_name.str());

            // 현재 스코프와 호출한 스코프가 같은 경우 바로 실행하기
            auto* current_scope = _getCurrentScope();
            if (compare(current_scope))
            {
                LOG_D("Call task (%s) in current scope", task_name.str());
                task.invoke();
                return ret;
            }

            // 현재 스코프가 존재할 경우
            if (current_scope)
            {
                // 현재 스코프가 동작하고 있는 작업이 동작하고자 하는 소크프에서 호출한 적이 있으면 순환 호출이 되기 때문에 오류
                auto& current_task = current_scope->_current_task;
                if (current_task.isInStack(_name))
                {
                    LOG_E("This Task is already in scope (%s)", name());
                    assert(false);

                }

                // 새로운 작업에 현재 작업하고 있는 작업을 호출한 모든 스코프와 대상 스코프를 넣는다.
                auto& current_scope_name = current_scope->_name;
                task.addStack(current_task, current_scope_name);
            }
            
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (startScope(task))
                {
                    pushToScope(std::move(task));
                }
                else
                {
                    ret->cancel();
                }
            }

            LOG_D("Call Task (%s)", task_name.str());
            _cv.notify_all();
            return ret;
        }

        inline const char* name() { return _name.str(); }
        inline ScopeState state() { return _state; }
        inline std::thread::id id() { return _id; }

    private:
        inline bool startScope(const ScopedTask& task) {
            if (ScopeState::IDLE == _state)
            {
                LOG_D("Task %s starts scope (%s)", task.name(), name());
                if (!_onStart(*this))
                {
                    LOG_D("Request manager is stopped.\n");
                    return false;
                }

                std::mutex local_mtx;
                std::unique_lock<std::mutex> lock(local_mtx);
                _cv.wait(lock, [&]() {
                    return _state == ScopeState::EMPTY;
                    });
            }
            return true;
        }
        inline void pushToScope(ScopedTask&& task) {
            if (_state == ScopeState::EMPTY)
                _state = ScopeState::RUNNING;
            _tasks.push(std::move(task));
        }
        

        
        inline bool compare(ScopedTaskScopeImpl* other) {
            if (other == nullptr)
                return false;
            return _id == other->_id;
        }

        void worker();
        

        Bn3Tag _name;

        std::function<ScopedTaskScopeImpl*()> _getCurrentScope;
        std::function<bool(ScopedTaskScopeImpl&)> _onStart;

        std::thread _thread;
        std::thread::id _id;

        ScopeState _state{ ScopeState::IDLE };
        ScopedTask _current_task;

        Bn3Queue(ScopedTask) _tasks; // allocated by bn3 allocator
        std::mutex _mtx;
        std::condition_variable _cv;

    };

    class ScopedTaskScopeImplPool
    {
    public:
        ScopedTaskScopeImplPool();
        ScopedTaskScopeImpl& getScope(const Bn3Tag& scope_name, std::function<bool(ScopedTaskScopeImpl&)> onStart);
        ScopedTaskScopeImpl* getCurrentScope();
        
        inline std::function<void()>& onClear() {
            return _onClear;
        }

    private:
        void clear();

        std::function<void()> _onClear;
        Bn3Deque(ScopedTaskScopeImpl) _scopes;

    };
}

#endif
