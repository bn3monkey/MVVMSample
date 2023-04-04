#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskHelper.hpp"

using namespace Bn3Monkey;

ScopedTaskScopeImpl::ScopedTaskScopeImpl(const char* scope_name,
    std::function<bool(ScopedTaskScopeImpl&)> onStart,
    std::function<ScopedTaskScopeImpl*()> getCurrentScope) :
    _onStart(onStart),
    _getCurrentScope(getCurrentScope)
{
    memcpy(_name, scope_name, 256);
}

ScopedTaskScopeImpl::ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other)
    : _onStart(std::move(other._onStart)),
      _getCurrentScope(std::move(other._getCurrentScope)),
      _thread(std::move(other._thread)),
      _id(std::move(other._id)),
      _state(std::move(other._state)),
      _current_task(std::move(other._current_task)),
      _tasks(std::move(other._tasks))
{
    memcpy(_name, other._name, 256);
    memset(other._name, 0, 256);
}

ScopedTaskScopeImpl::~ScopedTaskScopeImpl()
{
}

void ScopedTaskScopeImpl::start()
{
    LOG_D("Scope (%s) Starts", _name);
    
    // Because atomicity is guaranteed when running or calling from outside, _mtx is not locked.
    if (_state == ScopeState::IDLE)
    {
        _state = ScopeState::EMPTY;
        _thread = std::thread([&]() {worker(); });
    }
    else
    {
        LOG_E("Scope (%s) is already started", _name);
        assert(true);
    }
}
void ScopedTaskScopeImpl::timeout()
{
    LOG_D("Scope (%s) Timeouts", _name);

    bool is_stopped{ false };
    {
        std::unique_lock<std::mutex> lock(_mtx);
        if (ScopeState::EMPTY == _state)
        {
            _state = ScopeState::IDLE;
            is_stopped = true;
        }
    }
        /*
        if (_mtx.try_lock())
        {
            if (ScopeState::EMPTY == _state)
            {
                _state = ScopeState::IDLE;
                _thread.join();
                _mtx.unlock();
            }
        }
        else
        {
            LOG_D("Scope (%s) timeouts but scope is being used", _name);
        }
        */

    if (is_stopped)
    {
        _cv.notify_all();
        _thread.join();
    }
}
void ScopedTaskScopeImpl::stop()
{
    LOG_D("Scope %s Ends", _name);
    
    bool is_stopped{ false };
    {
        std::unique_lock<std::mutex> lock(_mtx);
        if (ScopeState::IDLE != _state)
        {
            _state = ScopeState::IDLE;
            is_stopped = true;
        }
    }

    if (is_stopped)
    {
        _cv.notify_all();
        _thread.join();
    }
}

void ScopedTaskScopeImpl::worker()
{
    LOG_D("Worker (%s) Start", _name);
    std::string thread_name = "scope ";
    thread_name += _name;
    setThreadName(thread_name.c_str());

    _id = std::this_thread::get_id();

    _cv.notify_all();

    for (;;)
    {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            using namespace std::chrono_literals;
            _current_task.clear();

            _cv.wait(lock, [&]() {
                return _state != ScopeState::EMPTY;
                });
            
            if (_state == ScopeState::IDLE)
            {
                LOG_D("Scope (%s) : Cancel all non-executed tasks ", _name);
                while (!_tasks.empty())
                {
                    _current_task = std::move(_tasks.front());
                    _tasks.pop();
                    _current_task.cancel();
                }
                LOG_D("Worker (%s) Ends", _name);
                break;
            }

            
            _current_task = std::move(_tasks.front());
            _tasks.pop();

            if (_tasks.empty())
            {
                _state = ScopeState::EMPTY;
                LOG_D("Scope(%s) - Tasks empty", _name);
            }
            else
            {
                _state = ScopeState::READY;
                LOG_D("Scope(%s) - Tasks remained", _name);
            }
        }

        LOG_D("Scope(%s) - Tasks(%s) start", _name, _current_task.name());
        _current_task.invoke();
        LOG_D("Scope(%s) - Tasks(%s) ends", _name, _current_task.name());
    }
}

/***********************************************/
ScopedTaskScopeImplPool::ScopedTaskScopeImplPool()
{
    _onTimeout = [&]() {timeout(); };
    _onClear = [&]() {clear(); };
}

ScopedTaskScopeImpl* ScopedTaskScopeImplPool::getScope(const char* scope_name, std::function<bool(ScopedTaskScopeImpl&)> onStart)
{
    auto iter = _scopes.find(scope_name);
    if (iter != _scopes.end())
    {
        return &(iter->second);
    }

    auto temp_getCurrentScope = [&]() {
        return getCurrentScope();
    };

    _scopes.emplace(scope_name, ScopedTaskScopeImpl(scope_name, onStart, temp_getCurrentScope));
    auto& ret = _scopes.at(scope_name);
    return &ret;
}
ScopedTaskScopeImpl* ScopedTaskScopeImplPool::getCurrentScope() {
    auto id = std::this_thread::get_id();
    for (auto& iter : _scopes) {
        if (id == iter.second.id())
            return &(iter.second);
    }
    return nullptr;
}
void ScopedTaskScopeImplPool::timeout()
{
    for (auto& iter : _scopes)
    {
        auto& scope = iter.second;
        scope.timeout();
    }
}
void ScopedTaskScopeImplPool::clear()
{
    for (auto& iter : _scopes)
    {
        auto& scope = iter.second;
        scope.stop();
    }
    _scopes.clear();
}