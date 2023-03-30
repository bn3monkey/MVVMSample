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
    enum class ScopeState
    {
        IDLE = 0, // 할당된 작업 없으면 쓰레드도 동작하지 않음
        EMPTY = 1, // 할당된 작업 없음
        READY = 2, // 할당된 작업 있음
        RUNNING = 3, // 작업 중임
        EXPIRED = 4, 
    };

    class ScopedTaskScopeImpl
    {
    public:

        ScopedTaskScopeImpl(const char* scope_name,
            std::function<bool(ScopedTaskScopeImpl&)> onStart,
            std::function<bool(ScopedTaskScopeImpl&)> onStop,
            std::function<ScopedTaskScopeImpl*()> getCurrentScope);

        ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other);

        virtual ~ScopedTaskScopeImpl();
       

        void start();
        void stop();
        void clear();

        template<class Func, class... Args>
        void run(const char* task_name, Func&& func, Args&&... args)
        {
            // Scope가 꺼져있으면 스코프 키기
            // 자기 자신 Scope에서 불렀으면 무조건 ScopeState는 Running
            // 다른 Scope에서 불렀으면 IDLE, EMPTY, READY, RUNNING 중 하나

            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            LOG_D("Make task (%s)", task_name);

            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (startScope(task))      
                    pushToScope(std::move(task));
            }

            LOG_D("Run Task (%s)", _name);
            _cv.notify_all();
        }

        template<class Func, class... Args>
        auto call(const char* task_name, Func&& func, Args&&... args) -> ScopedTaskResultImpl<std::result_of_t<Func(Args...)>>
        {
            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            auto ret = task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            LOG_D("Make task (%s)", task_name);

            // 현재 스코프와 호출한 스코프가 같은 경우 바로 실행하기
            auto* current_scope = _getCurrentScope();
            if (compare(current_scope))
            {
                LOG_D("Call task (%s) in current scope", task_name);
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
                    LOG_E("This Task is already in scope (%s)", _name);
                    assert(false);

                }

                // 새로운 작업에 현재 작업하고 있는 작업을 호출한 모든 스코프와 대상 스코프를 넣는다.
                // 더 이상 스코프를 추가할 수 없으면 오류를 뱉는다.
                auto* current_scope_name = current_scope->_name;
                if (!task.addStack(current_task, current_scope_name))
                {
                    LOG_E("This Task cannot be added to scope %s", current_scope_name);
                    assert(false);
                }
            }
            
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (startScope(task))
                {
                    pushToScope(std::move(task));
                }
                else
                {
                    ret.invalidate();
                }
            }

            LOG_D("Call Task (%s)", _name);
            _cv.notify_all();
            return ret;
        }

        inline const char* name() { return _name; }
        inline ScopeState state() { return _state; }
        inline std::thread::id id() { return _id; }

    private:
        inline bool startScope(const ScopedTask& task) {
            if (ScopeState::IDLE == _state)
            {
                LOG_D("Task %s starts scope (%s)", task.name(), _name);
                _state = ScopeState::EMPTY;
                bool ret = _onStart(*this);
                if (!ret)
                {
                    LOG_D("Task Runner is not initialized");
                    _state = ScopeState::IDLE;
                    return false;
                }
            }
            return true;
        }
        inline void pushToScope(ScopedTask&& task) {
            if (_state == ScopeState::EMPTY)
                _state = ScopeState::READY;
            _tasks.push(std::move(task));
        }
        

        
        inline bool compare(ScopedTaskScopeImpl* other) {
            if (other == nullptr)
                return false;
            return _id == other->_id;
        }

        void worker();
        

        char _name[256]{ 0 };

        std::function<bool(ScopedTaskScopeImpl&)> _onStart;
        std::function<bool(ScopedTaskScopeImpl&)> _onStop;
        std::function<ScopedTaskScopeImpl*()> _getCurrentScope;

        std::thread _thread;
        std::thread::id _id;

        ScopeState _state{ ScopeState::IDLE };
        ScopedTask _current_task;
        std::queue<ScopedTask> _tasks;
        std::mutex _mtx;
        std::condition_variable _cv;

    };

    class ScopedTaskScopeImplPool
    {
    public:
        ScopedTaskScopeImpl* getScope(const char* scope_name,
            std::function<bool(ScopedTaskScopeImpl&)> onStart,
            std::function<bool(ScopedTaskScopeImpl&)> onStop);
        ScopedTaskScopeImpl* getCurrentScope();
        void clearScope();
        void clear() {
            _scopes.clear();
        }

    private:
        std::unordered_map<std::string, ScopedTaskScopeImpl> _scopes;
    };
}

#endif
