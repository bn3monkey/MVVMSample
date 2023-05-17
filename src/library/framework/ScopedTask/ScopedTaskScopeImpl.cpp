#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskHelper.hpp"
#include "ScopedTaskScopeImpl.hpp"

using namespace Bn3Monkey;

ScopedTaskScopeImpl::ScopedTaskScopeImpl(const Bn3Tag& scope_name,
    std::function<ScopedTaskScopeImpl*()> getCurrentScope,
    std::function<bool()> is_pool_initialized) :
    _name(scope_name),
    _getCurrentScope(getCurrentScope),
    _is_pool_initialized(is_pool_initialized)
{
    _tasks = Bn3Queue(ScopedTask) { Bn3QueueAllocator(ScopedTask, Bn3Tag("tasks_", _name)) };

}

ScopedTaskScopeImpl::ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other)
    : 
      _name(other._name),
      _getCurrentScope(std::move(other._getCurrentScope)),
      _is_pool_initialized(std::move(other._is_pool_initialized)),
      _thread(std::move(other._thread)),
      _id(std::move(other._id)),
      _state(std::move(other._state)),
      _current_task(std::move(other._current_task)),
      _tasks(std::move(other._tasks))
{

}

ScopedTaskScopeImpl::~ScopedTaskScopeImpl()
{
}

bool ScopedTaskScopeImpl::start()
{
    if (ScopeState::IDLE == _state)
    {
        LOG_D("Scope (%s) starts", _name.str());

        if (!_is_pool_initialized())
        {
            LOG_D("Scope (%s) cannot be started", _name.str());
            return false;
        }

        _thread = std::thread{ &ScopedTaskScopeImpl::worker, this };
        _thread.detach();

        {
            std::mutex local_mtx;
            std::unique_lock<std::mutex> lock(local_mtx);
            _cv.wait(lock, [&]() {
                return ScopeState::EMPTY == _state;
                });
        }
    }
    return true;
}

void ScopedTaskScopeImpl::push(ScopedTask&& task)
{
    _tasks.push(std::move(task));
    if (ScopeState::EMPTY == _state)
    {
        _state = ScopeState::RUNNING;
    }
}

void ScopedTaskScopeImpl::stop()
{
    LOG_D("Scope (%s) stops", _name.str());
    
    bool is_stopped{ false };
    {
        std::unique_lock<std::mutex> lock(_mtx);
        if (ScopeState::IDLE != _state)
        {
            _state = ScopeState::STOPPING;
            is_stopped = true;
        }

    }


    if (is_stopped)
    {
        _cv.notify_all();
        {
            std::mutex local_mtx;
            std::unique_lock<std::mutex> lock(local_mtx);
            _cv.wait(lock, [&]() {
                return ScopeState::IDLE == _state;
                });
        }
    }
}

void ScopedTaskScopeImpl::worker()
{
    LOG_D("Worker (%s) Start", _name.str());
    std::string thread_name = "scope ";
    thread_name += _name.str();
    setThreadName(thread_name.c_str());

    _id = std::this_thread::get_id();

    _state = ScopeState::EMPTY;
    _cv.notify_all();

    for (;;)
    {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            using namespace std::chrono_literals;
            _current_task.clear();

            bool timeout = _cv.wait_for(lock, 10s, [&]() {
                return _state != ScopeState::EMPTY;
                });

            if (!timeout)
            {
                if (_state == ScopeState::EMPTY)
                {
                    _state = ScopeState::STOPPING;
                }
            }

            if (ScopeState::STOPPING == _state)
            {
                LOG_D("Scope (%s) : Cancel all non-executed tasks ", _name.str());
                while (!_tasks.empty())
                {
                    _current_task = std::move(_tasks.front());
                    _tasks.pop();
                    _current_task.cancel();
                }
                LOG_D("Worker (%s) Ends", _name.str());
                _state = ScopeState::IDLE;
                break;
            }

            
            _current_task = std::move(_tasks.front());
            _tasks.pop();

            if (_tasks.empty())
            {
                _state = ScopeState::EMPTY;
                LOG_D("Scope(%s) - Tasks empty", _name.str());
            }
            else
            {
                _state = ScopeState::RUNNING;
                LOG_D("Scope(%s) - Tasks remained", _name.str());
            }
        }

        LOG_D("Scope(%s) - Tasks(%s) start", _name.str(), _current_task.name());
        _current_task.invoke();
        LOG_D("Scope(%s) - Tasks(%s) ends", _name.str(), _current_task.name());
    }

    _cv.notify_all();
}

/***********************************************/
ScopedTaskScopeImplPool::ScopedTaskScopeImplPool()
{
    _scopes = Bn3Deque(ScopedTaskScopeImpl) { Bn3DequeAllocator(ScopedTaskScopeImpl, Bn3Tag("scopes")) };
    _is_pool_initialized = std::bind(&ScopedTaskScopeImplPool::isPoolInitialized_, this);
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _is_initialized = true;
    }
}

ScopedTaskScopeImpl& ScopedTaskScopeImplPool::getScope(const Bn3Tag& scope_name)
{
    for (auto& scope : _scopes)
    {
        if (scope._name == scope_name)
            return scope;
    }

    auto temp_getCurrentScope = [&]() {
        return getCurrentScope();
    };

    _scopes.emplace_back(scope_name, temp_getCurrentScope, isPoolInitialized());
    return _scopes.back();
}
ScopedTaskScopeImpl* ScopedTaskScopeImplPool::getCurrentScope() {
    auto id = std::this_thread::get_id();
    for (auto& scope : _scopes)
    {
        if (id == scope._id)
            return &scope;
    }
    return nullptr;
}
void ScopedTaskScopeImplPool::release()
{
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _is_initialized = false;
    }

    for (auto& scope : _scopes)
    {
        scope.stop();
    }
    _scopes.clear();
}