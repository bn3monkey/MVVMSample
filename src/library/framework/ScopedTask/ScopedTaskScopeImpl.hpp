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
    class ScopedTaskScope;
    class ScopedTaskRunnerImpl;

    enum class ScopeStatus
    {
        IDLE = 0, // 할당된 작업 없으면 쓰레드도 동작하지 않음
        EMPTY = 1, // 할당된 작업 없음
        READY = 2, // 할당된 작업 있음
        RUNNING = 3, // 작업 중임
    };

    class ScopedTaskScopeImpl
    {
    public:
        
        explict ScopedTaskScopeImpl(const char* scope_name, 
            std::function<void(ScopedTaskScopeImpl&)> onStart,
            std::function<void(ScopedTaskScopeImpl&)> onStop);
        virtual ~ScopedTaskScopeImpl();
        
        void start()
        {
            _state = ScopeState::EMPTY;
            _thread = _thread([&](){worker()});
        }
        void stop()
        {
            _state = ScopeState::IDLE;
            _id = std::thread::id();
            
            _thread.join();
        }

        template<class Func, class... Args>
        void run(const char* task_name, Func&& func, Args&&.. args)
        {
            // Scope가 꺼져있으면 스코프 키기

            _onStart(*this);

            // 생성된 ScopeTask의 onTaskFinished에 
            // 아무것도 없음

            // 생성된 ScopeTask의 onTaskWaiting에
            // 아무것도 없음

            // ScopeTask 수행 요청하기
        }  

        template<class Func, class... Args>
        std::shared_ptr<ScopedTaskScopeImpl> call(const char* task_name, Func&& func, Args&&... args)
        {
            // Scope가 꺼져있으면 스코프 키기
            _onStart(*this);
            
            // 현재 스코프와 이 소코프가 같으면 바로 작동시키기
            
            // 현재 스코프의 call stack에 이 스코프가 있으면 순환 호출이 되므로 오류 메세지와 함께 반려시키기

            // 현재 스코프의 calling Scope에 이 스코프를 넣기
            // 이 스코프의 called Scope에 현재 스코프를 넣기 

            // 생성된 ScopeTask의 onTaskFinished에 
            // 2. 이 스코프의 result_cv에 알람주기

            // 생성된 ScopeTask의 onTaskWaiting에
            // 1. 이 스코프의 result_cv에서 기다리기
            // 2. 현재 스코프의 calling Scope에 이 스코프를 빼는 것과
            // 이 스코프의 called Scope에 현재 스코프를 빼기
            
            // ScopeTask 수행 요청하기
        } 

        inline const char* name() { return _name; }

    private:
        friend ScopedTaskScopeImpl& ScopedTaskScope::getScope(const char* scope_name); 

    private:
        ScopedTaskScopeImpl* getCurrentScope() {
            auto id = std::this_thread::get_id();
            for (auto& iter : _scopes) {
                if (id == iter.second._id)
                    return &(iter.second);
            }
            return nullptr;
        }

        static std::unorderd_map<std::string, ScopedTaskScopeImpl> _scopes;
        
        char _name[256] {0};
        ScopeStatus _state {ScopeStatus::IDLE};

        std::function<void(ScopedTaskScopeImpl&)> _onStart;
        std::function<void(ScopedTaskScopeImpl&)> _onstop;

        std::thread _thread;
        std::thread::id _id;

        std::queue<ScopedTask> _tasks;
        std::mutex _mtx;
        std::condition_variable _cv;
        
        void worker();
    }

    /*
    class ScopedTaskScopeImpl
    {
    public:
        explicit ScopedTaskScopeImpl(const char* name, std::function<void(ScopedTaskScopeImpl&)> onTimeout);
        virtual ~ScopedTaskScopeImpl();
        

        // @brief TaskScope의 현재 상태를 받아온다.
        ScopeStatus getStatus();
        inline const char* getName() const {return _name; }
        inline std::thread::id getID() cosnt { return _id;}
        
        // Task Scope를 생성한 쓰레드에서 이 작업을 수행해야 한다.
        
        // @brief TaskScope가 수행하는 쓰레드가 멈춰있으면 가동시킨다. TaskScope에 새로운 작업을 할당한다.
        void run(const char* task_name, std::function<void()> task);

        // @brief TaskScope가 수행하는 쓰레드가 멈춰있으면 가동시킨다. TaskScope에 새로운 작업을 할당한 뒤, 그 작업이 끝날 때까지 기다린다.
        void call(const char* task_name, TaskScopeImpl& called_scope, std::function<void()> task);

        // @brief TaskScope가 수행하는 쓰레드를 멈춘다.
        void stop();
        

    private:
        // 스코프의 이름
        const char* _name;

        // 현재 수행하고 있는 task의 이름
        char _task_name[256] = {0};
        // 이 Scope를 호출한 Scope
        TaskScopeImpl* _called_scope {nullptr};
        // 이 Scope가 호출한 Scope
        TaskScopeImpl* _calling_scope {nullptr};

        // 현재 이 스코프에 배당된 작업들
        std::atomic<ScopeStatus> _status;

        std::queue<NamedTask> _tasks;
        std::mutex _task_mtx;
        std::condition_variable _task_cv;
        
        std::thread _worker;
        std::thread::id _worker_id;

        // 일정 시간이 지나면 이 쓰레드를 어플리케이션 쓰레드 목록에서 내려 버린다.
        const std::chrono::seconds timeout {5};
        std::function<void(TaskScopeImpl&)> _onTimeout;

        // @brief TaskScope에 할당된 작업을 수행한다.
        void worker();
    };
    */
}
#endif
