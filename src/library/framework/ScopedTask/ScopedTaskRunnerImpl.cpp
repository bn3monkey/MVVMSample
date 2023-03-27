#include "ScopedTaskRunnerImpl.hpp"
#include "ScopedTaskHelper.hpp"

using namespace Bn3Monkey;

bool ScopedTaskRunnerImpl::initialize()
{
    LOG_D("ScopedTaskRunner initializes");
    _is_running = true;
    _thread = std::thread(manager);
}

void ScopedTaskRunnerImpl::release()
{    
    LOG_D("ScopedTaskRunner releases");
    _is_running = false;
    _request_cv.notify_all();
    _thread.join();
}
void ScopedTaskRunnerImpl::start(ScopedTaskScopeImpl& task_scope)
{
    LOG_D("Request manager to start task scope (%s)", task_scope.name());
    {
        std::unique_lock<std::mutex> lock(_response_mtx);
        _is_request_done = false;

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _requests.emplace(&task_scope, true);
        }
        _request_cv.notify_all();

        _response_cv.wait(lock, [&]() {
            return _is_request_done;
        });
    }
}
void ScopedTaskRunnerImpl::stop(ScopedTaskScopeImpl& task_scope)
{
    LOG_D("Request manager to stop task scope (%s)", task_scope.name());
    {
        std::unique_lock<std::mutex> lock(_response_mtx);
        _is_request_done = false;

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _requests.emplace(&task_scope, false);
        }
        _request_cv.notify_all();

        _response_cv.wait(lock, [&]() {
            return _is_request_done;
            });
    }
}

void ScopedTaskRunnerImpl::manager()
{
    LOG_D("Manager starts");
    setThreadName("ST_Manager");

    for (;;)
    {
        Request request;

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _request_cv.wait(lock, [&]() {
                return !_is_running || !_requests.empty();
                });
            if (!_is_running)
            {
                {
                    std::unique_lock<std::mutex> lock(_response_mtx);
                    _is_request_done = true;
                }
                _response_cv.notify_one();
                return;
            }
            request = std::move(_requests.front());
            _requests.pop();
        }

        if (request.is_activated)
        {
            LOG_D("Manager starts Worker (%s)", request.scope->name());
            request.scope->start();
        }
        else {
            LOG_D("Manager stops Worker (%s)", request.scope->name());
            request.scope->stop();
        }

        {
            std::unique_lock<std::mutex> lock(_response_mtx);
            _is_request_done = true;
        }
        _response_cv.notify_one();
    }
}