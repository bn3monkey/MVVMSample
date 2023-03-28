#ifndef __BN3MONKEY_TASK_SCOPE_IMPL__
#define __BN3MONKEY_TASK_SCOPE_IMPL__

#include "ScopedTask.hpp"

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

#define BN3MONKEY_DEBUG

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
        WAITING = 4, // 
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

        template<class Func, class... Args>
        void run(const char* task_name, Func&& func, Args&&... args)
        {
            // Scope가 꺼져있으면 스코프 키기
            // 자기 자신 Scope에서 불렀으면 무조건 ScopeState는 Running
            // 다른 Scope에서 불렀으면 IDLE, EMPTY, READY, RUNNING 중 하나

            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (ScopeState::IDLE == _state)
                {
                    bool ret = _onStart(*this);
                    if (!ret)
                    {
                        LOG_D("Task Runner is not initialized");
                        return;
                    }
                    _cv.wait(lock, [&]() {
                        return _state == ScopeState::EMPTY;
                        });
                }

                _tasks.push(task);
                if (_state == ScopeState::EMPTY)
                    _state = ScopeState::READY;
            }

            _cv.notify_all();
        }

        template<class Func, class... Args>
        auto call(const char* task_name, Func&& func, Args&&... args) -> ScopedTaskResultImpl<decltype(func(args...))>
        {
            // ScopeTask 수행 요청하기
            ScopedTask task{ task_name };
            auto ret = task.make(std::forward<Func>(func), std::forward<Args>(args)...);

            // 현재 스코프와 호출한 스코프가 같은 경우 바로 실행하기
            auto* current_scope = _getCurrentScope();
            if (compare(current_scope))
            {
                task.invoke();
                return ret;
            }
            
            // 현재 스코프와 이 스코프가 같지 않은 경우
            {
                // Scope가 꺼져있으면 스코프 키기
                std::unique_lock<std::mutex> lock(_mtx);
                if (ScopeState::IDLE == _state)
                {
                    bool ret = _onStart(*this);
                    if (!ret)
                    {
                        LOG_D("Task Runner is not initialized");
                        assert(false);
                    }
                    _cv.wait(lock, [&]() {
                        return _state == ScopeState::EMPTY;
                        });
                }
                                
                if (current_scope)
                {
                    auto& current_task = current_scope->_current_task;
                    if (current_task.isInStack(_name))
                    {
                        LOG_E("This Task is already in scope (%s)", _name);
                        assert(false);

                    }
                    
                    auto* current_scope_name = current_scope->_name;
                    if (!task.addStack(current_task, current_scope_name))
                    {
                        LOG_E("This Task cannot be added to scope %s", current_scope_name);
                        assert(false);
                    }
                }

                _tasks.push(task);
                if (_state == ScopeState::EMPTY)
                    _state = ScopeState::READY;
                else if (_state == ScopeState::IDLE)
                {
                    LOG_E("Scope(%s) State is not empty when after starting scope.", _name);
                    assert(false);
                }
            }

            _cv.notify_all();
            return ret;
        }

        inline const char* name() { return _name; }
        inline ScopeState state() { return _state; }

    private:
        
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

    private:
        std::unordered_map<std::string, ScopedTaskScopeImpl> _scopes;
    }
}

#endif
