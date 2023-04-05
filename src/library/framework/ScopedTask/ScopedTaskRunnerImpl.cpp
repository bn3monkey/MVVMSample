#include "ScopedTaskRunnerImpl.hpp"
#include "ScopedTaskHelper.hpp"

using namespace Bn3Monkey;

bool ScopedTaskRunnerImpl::initialize(std::function<void()> onStop)
{
    LOG_D("ScopedTaskRunner initializes");

    _onStart = [&](ScopedTaskScopeImpl& task_scope) {
        return start(task_scope);
    };
    _onStop = onStop;
    
    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        _is_running = true;
    }
    _thread = std::thread([&]() {manager(); });
    return true;
}

void ScopedTaskRunnerImpl::release()
{    
    LOG_D("ScopedTaskRunner releases");
    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        _is_running = false;
    }
    _request_cv.notify_all();
    _thread.join();
}
bool ScopedTaskRunnerImpl::start(ScopedTaskScopeImpl& task_scope)
{
    LOG_D("Request manager to start task scope (%s)", task_scope.name());

    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        if (!_is_running)
        {
            LOG_D("Request manager is stopped");
            return false;
        }

        _requests.emplace(&task_scope, 1);
    }

    _request_cv.notify_all();
    return true;
}


void ScopedTaskRunnerImpl::manager()
{
    LOG_D("Manager starts");
    setThreadName("ST_Manager");

    for (;;)
    {
        Request request;
        bool start{ false };

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _request_cv.wait(lock, [&]() {
                return !_is_running || !_requests.empty();
                });
            
            if (!_is_running)
            {
                break;
            }
            
            request = std::move(_requests.front());
            _requests.pop();
            
        }

        if (request.request_num == 1)
        {
            LOG_D("Manager starts Worker (%s)", request.scope->name());
            request.scope->start();
        }
    }    

    LOG_D("Manager stop all workers\n");
    _onStop();
}