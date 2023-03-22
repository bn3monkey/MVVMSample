#ifndef __BN3MONKEY_TASK_RUNNER_IMPL__
#define __BN3MONKEY_TASK_RUNNER_IMPL__

#include "TaskScopeImpl.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <chrono>

#include <vector>

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
    class TaskRunnerImpl
    {
    public:
        void initialize()
        {
        }
        void release()
        {
        }

        template<class F, class Args..>
        void run(TaskScopeImpl& scope, const char* task_name, F&& f, Args&&... args)
        {
            auto bindend_function = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            requestRun(scope, task_name, binded_function);
        }

        template<class F, class Args...>
        TaskResult<std::result_of<F(Args...)>::type> call(TaskScopeImpl& scope, const char* task_name, F&& f, Args&&... args)
        {
            using ReturnType = std::result_of<F(Args...)>::type;
            
            auto binded_function = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            TaskResultImpl<ReturnType>* taskResult = new TaskResultImpl();

            auto binded_function_with_result = [&binded_function, &taskResult]() {
                taskResult->result = binded_function();
                taskResult->is_finished = true;
                taskResult->cv.notify_all();
            };

            // TaskRunner와 call을 호출한 thread 사이에서 결과를 공유할 TaskResultImpl 객체를 만드는 것이 중요하다.
            auto w_called_scope = getCurrentScope();
            if (auto called_scope = w_called_scope.lock())
            {
                requestCall(scope, *called_scope, task_name, binded_function_with_result);    
            }
            else
            {
                requestRun(scope, task_name, binded_function_with_result);   
            }
        }



    private:
        bool is_running;
        std::vector<std::shared_ptr<TaskScopeImpl>> _scopes;
        std::queue<std::function<void()>> _requests;
        std::mutex _request_mtx;
        std::condition_variable _request_cv;

        std::weak_ptr<TaskScopeImpl> getCurrentScope()
        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            for (auto& scope : _scopes)
            {
                if (scope->getId() == std::this_thread::get_id())
                {
                    return scope;
                }
            }
            return nullptr;
        }
        std::weak_ptr<TaskScopeImpl> requestCreate(const char* name) {
            {
                std::unique_lock<std::mutex> lock(_request_mtx);
                for (auto& scope : _scopes)
                {
                    if (scope->name == name)
                    {
                        return scope;
                    }
                }
                auto scope = std::make_shared<TaskScopeImpl>(name, [](Bn3Monkey::TaskScopeImpl& impl) {
                    requestStop(impl);
                })
                _scopes.push_back(name);
                return scope;
            }
        }
        void requestRun(TaskScopeImpl& impl, const char* task_name, std::function<void()> binded_function) {
            {
                std::unique_lock<std::mutex> lock(_request_mtx);
                _requests.push([&impl, &task_name, &binded_function](){
                    impl.run(task_name, binded_function);
                });
            }
            _request_cv.notify_one();
        }
        void requestCall(TaskScopeImpl& impl, TaskScopeImpl& other, const char* task_name, std::function<void()> binded_function) {
            {
                std::unique_lock<std::mutex> lock(_request_mtx);
                _requests.push([&impl, &other, &task_name, &binded_function](){
                    impl.call(other, task_name, binded_function);
                });
            }
            _request_cv.notify_one();
        }
        void requestStop(TaskScopeImpl& impl) {
            {
                std::unique_lock<std::mutex> lock(_request_mtx);
                _requests.push([&impl](){
                    impl.stop();
                });
            }
            _request_cv.notify_one();
        }

        void routine()
        {
            // 디버깅을 위해 Thread의 이름을 설정한다.
            setThreadName("Task Runner");

            is_running = true;

             LOG_V("Runner Starts");

            for (;;)
            {
                std::function<void()> request;
                {
                    std::unique_lock<std::mutex> lock(_request_mtx);
                   _request_cv.wait(lock, [&]() {
                        return !is_running || !_requests.empty();
                   })

                    // 쓰레드를 더 이상 동작시키지 않으면 쓰레드 목록에서 내린다.
                    if (!is_running)
                    {
                        break;
                    }

                    request = std::move(_requests.front());
                    request.pop();
                }

                request();

            }
        }
    };
}