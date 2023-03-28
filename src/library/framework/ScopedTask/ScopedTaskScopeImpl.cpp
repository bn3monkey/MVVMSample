#include "ScopedTaskScopeImpl.hpp"

std::unordered_map<std::string, Bn3Monkey::ScopedTaskScopeImpl> Bn3Monkey::ScopedTaskScopeImpl::_scopes;

using namespace Bn3Monkey;

ScopedTaskScopeImpl::ScopedTaskScopeImpl(const char* scope_name,
    std::function<bool(ScopedTaskScopeImpl&)> onStart,
    std::function<bool(ScopedTaskScopeImpl&)> onStop,
    std::function<ScopedTaskScopeImpl*()> getCurrentScope) :
    _onStart(onStart),
    _onStop(onStop),
    _getCurrentScope(getCurrentScope)
{
    memcpy(_name, scope_name, 256);
}

ScopedTaskScopeImpl::ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other)
    : _onStart(std::move(other._onStart)),
      _onStop(std::move(other._onStop)),
      _getCurrentScope(std::move(other._getCurrentScope)),
      _thread(std::move(other._thread)),
      _id(std::move(other._id)),
      _state(std::move(other._state)),
      _current_task(std::move(other._current_task)),
      _tasks(std::move(other._tasks)),
      _calling_scope(std::move(other._calling_scope)),
    _called_scope(std::move(other._called_scope))
{
    memcpy(_name, other._name, 256);
    memset(other._name, 0, 256);
}

virtual ScopedTaskScopeImpl::~ScopedTaskScopeImpl()
{
}

void ScopedTaskScopeImpl::start()
{
    if (_state == ScopeState::IDLE)
    {
        _state = ScopeState::EMPTY;
        _thread = std::thread([&]() {worker(); });
    }
}
void ScopedTaskScopeImpl::stop()
{
    if (_state != ScopeState::IDLE)
    {
        _state = ScopeState::IDLE;
        _id = std::thread::id();
        _cv.notify_all();
        _thread.join();
    }
}

void ScopedTaskScopeImpl::worker()
{
    LOG_D("Worker (%s) Start", _name);
    _id = std::this_thread::get_id();
    _cv.notify_all();
    for (;;)
    {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            using namespace std::chrono_literals;
            bool is_timeout = _cv.wait_for(lock, 10s, [&]() {
                return _state != ScopeState::EMPTY;
                });
            if (!is_timeout && _tasks.empty())
            {
                LOG_D("Worker (%s) : time out", _name);
                bool ret = _onStop(*this);
                if (!ret)
                {
                    LOG_D("Task Runner is not initialized");
                    return;
                }
                _cv.wait(lock, [&]() {
                    return _state == ScopeState::IDLE;
                    });
            }
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
            _state = ScopeState::RUNNING;
            _current_task = std::move(_tasks.front());
            _tasks.pop();
        }
        LOG_D("Scope(%s) - Tasks(%s) start", _name, _current_task.name());
        _current_task.invoke();
        LOG_D("Scope(%s) - Tasks(%s) ends", _name, _current_task.name());
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if (_tasks.empty())
            {
                _state = ScopeState::EMPTY;
                LOG_D("Scope(%s) - Tasks all executed", _name);
            }
            else
            {
                _state = ScopeState::READY;
                LOG_D("Scope(%s) - Tasks remained", _name);
            }
        }
    }
}

/***********************************************/

ScopedTaskScopeImpl* ScopedTaskScopeImplPool::getScope(const char* scope_name,
    std::function<bool(ScopedTaskScopeImpl&)> onStart,
    std::function<bool(ScopedTaskScopeImpl&)> onStop)
{
    auto iter = _scopes.find(scope_name);
    if (iter != _scopes.end())
    {
        return &(iter->second);
    }
    _scopes.emplace(scope_name, ScopedTaskScopeImpl(scope_name, onStart, onStop));
    auto& ret = _scopes.at(scope_name);
    return &ret;
}
ScopedTaskScopeImpl* ScopedTaskScopeImplPool::getCurrentScope() {
    auto id = std::this_thread::get_id();
    for (auto& iter : _scopes) {
        if (id == iter.second._id)
            return &(iter.second);
    }
    return nullptr;
}
void ScopedTaskScopeImplPool::clearScope()
{
    for (auto& iter : _scopes)
    {
        iter.second.stop();
    }
}