#include "ScopedTaskRunnerImpl.hpp"
#include "ScopedTaskHelper.hpp"

using namespace Bn3Monkey;

bool ScopedTaskRunnerImpl::initialize()
{
    LOG_V("ScopedTaskRunner initializes");
    is_running = true;
    _thread = std::thread(manager);
}

void ScopedTaskRunnerImpl::release()
{    
    LOG_V("ScopedTaskRunner releases");
    _is_running = false;
    _thread.join();
}
void ScopedTaskRunnerImpl::start(ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to start task scope (%s)", task_scope.name());
    {
        std::unique_lock lock(_request_mtx);
        _requests.emplace(task_scope, true);
    }
    _request_cv.notfiy_all();
}
void ScopedTaskRunnerImpl::stop(ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to stop task scope (%s)", task_scope.name());
    {
        std::unique_lock lock(_request_mtx);
        _requests.emplace(task_scope, false);
    }
    _request_cv.notfiy_all();
}

void ScopedTaskRunnerImpl::manager()
{
    LOG_V("Manager starts")
    setThreadName("ST_Manager");

    for (;;)
    {
        Request request;
        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _request_cv.wait(_request_mtx, [&](){
                return !_is_running || !requests.empty(); 
            });
        }
        if (!_is_running)
            return
        request = std::move(_requests.front());
        _requests.pop()
        if (request.is_activated)
        {
            LOG_V("Manager starts Worker (%s)", request.scope.name());
            request.scope.start();
        }
        else {
            LOG_V("Manager starts Worker (%s)", request.scope.name());
            request.scope.stop();
        }
    }
}