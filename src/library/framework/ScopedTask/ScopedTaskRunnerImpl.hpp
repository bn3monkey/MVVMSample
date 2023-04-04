#ifndef __BN3MONKEY_TASK_RUNNER_IMPL__
#define __BN3MONKEY_TASK_RUNNER_IMPL__

#include "ScopedTaskImpl.hpp"
#include "ScopedTaskScopeImpl.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <chrono>
#include <vector>
#include <unordered_map>

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
    class ScopedTaskRunnerImpl
    {
    public:
        bool initialize(
            std::function<void()> onTimeout,
            std::function<void()> onStop);
        void release();
        inline std::function<bool(ScopedTaskScopeImpl&)>& onStart() {
            return _onStart;
        }

    private:

        bool start(ScopedTaskScopeImpl& task_scope);

        struct Request {
            ScopedTaskScopeImpl* scope {nullptr};
            Request() {}
            Request(ScopedTaskScopeImpl* scope) : scope(scope) {}
        };

        std::thread _thread;
        std::queue<Request> _requests;
        bool _is_running;
        std::mutex _request_mtx;
        std::condition_variable _request_cv;

        std::function<bool(ScopedTaskScopeImpl&)> _onStart;
        std::function<void()> _onTimeout;
        std::function<void()> _onStop;
        void manager();
        
    };
}

#endif //__BN3MONKEY_TASK_RUNNER_IMPL__ 