#ifndef __BN3MONKEY_TASK_RUNNER_IMPL__
#define __BN3MONKEY_TASK_RUNNER_IMPL__

#include "ScopedTaskHelper.hpp"
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
#include <unorderd_map>

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
    class ScopedTaskRunnerImpl
    {
    public:
        bool initialize();
        void release();
        void start(ScopedTaskScopeImpl& task_scope);
        void stop(ScopedTaskScopeImpl& task_scope);

    private:
        struct Request {
            Bn3Monkey::ScopedTaskScopeImpl* scope {nullptr};
            bool is_activated{false};
            bool* is_done{ nullptr };
            Request() {}
            Request(Bn3Monkey::ScopedTaskScopeImpl* scope, bool is_activated, bool* is_done) : scope(scope), is_activated(is_activated), is_done(is_done) {}
        };

        std::thread _thread;
        std::queue<Request> _requests;
        bool _is_running;
        std::mutex _request_mtx;
        std::condition_variable _request_cv;

        void manager();
        
    };
}