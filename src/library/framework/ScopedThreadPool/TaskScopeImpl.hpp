#ifndef __BN3MONKEY_TASK_SCOPE_IMPL__
#define __BN3MONKEY_TASK_SCOPE_IMPL__

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
    class TaskScopeImpl
    {
    public:
        explicit TaskScopeImpl(const char* name, std::function<void(TaskScopeImpl&)> onTimeout);
        virtual ~TaskScopeImpl();
        enum class ScopeStatus
        {
            IDLE = 0, // 할당된 작업 없으면 쓰레드도 동작하지 않음
            EMPTY = 1, // 할당된 작없 없음
            READY = 2, // 할당된 작업 있음
            WORKING = 3, // 작업 중임
            WAITING = 4, // 다른 작업이 끝날 때까지 기다리는 중
        };

        // @brief TaskScope의 현재 상태를 받아온다.
        ScopeStatus getStatus();
        inline const char* getName() const {return _name; }
        inline std::thread::id getId() cosnt { return _id;}
        
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
        char task_name[256] = {0};
        // 이 Scope를 호출한 Scope
        TaskScopeImpl* _called_scope {nullptr};
        // 이 Scope가 호출한 Scope
        TaskScopeImpl* _calling_scope {nullptr};

        // 현재 이 스코프에 배당된 작업들
        ScopeStatus _status;

        struct NamedTask {
            char name[256] = {0};
            std::function<void()> task;
        }

        std::queue<NamedTask> _tasks;
        std::mutex _task_mtx;
        std::condition_variable _task_cv;
        
        std::thread _worker;
        std::thread::id _worker_id;
        bool is_running;

        // 일정 시간이 지나면 이 쓰레드를 어플리케이션 쓰레드 목록에서 내려 버린다.
        const std::chrono::seconds timeout {5};
        std::function<void(TaskScopeImpl&)> _onTimeout;

        // @brief TaskScope에 할당된 작업을 수행한다.
        void worker();
    };
}
#endif
