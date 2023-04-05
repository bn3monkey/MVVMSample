#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskHelper.hpp"

using namespace Bn3Monkey;

ScopedTaskScopeImpl::ScopedTaskScopeImpl(const Bn3Tag& scope_name,
    std::function<bool(ScopedTaskScopeImpl&)> onStart,
    std::function<ScopedTaskScopeImpl*()> getCurrentScope) :
    _name(scope_name),
    _onStart(onStart),
    _getCurrentScope(getCurrentScope)
{
    _tasks = Bn3Queue(ScopedTask) { Bn3QueueAllocator(ScopedTask, Bn3Tag("tasks_", _name)) };
}

ScopedTaskScopeImpl::ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other)
    : 
      _name(other._name),
      _onStart(std::move(other._onStart)),
      _getCurrentScope(std::move(other._getCurrentScope)),
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

void ScopedTaskScopeImpl::start()
{
    LOG_D("Scope (%s) starts", _name.str());
    _thread = std::thread([&]() {worker(); });
}

void ScopedTaskScopeImpl::stop()
{
    LOG_D("Scope (%s) stops", _name.str());
    
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

            _cv.wait(lock, [&]() {
                return _state != ScopeState::EMPTY;
                });

            if (_state == ScopeState::IDLE)
            {
                LOG_D("Scope (%s) : Cancel all non-executed tasks ", _name.str());
                while (!_tasks.empty())
                {
                    _current_task = std::move(_tasks.front());
                    _tasks.pop();
                    _current_task.cancel();
                }
                LOG_D("Worker (%s) Ends", _name.str());
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
}

/***********************************************/
ScopedTaskScopeImplPool::ScopedTaskScopeImplPool()
{
    _onClear = [&]() {clear(); };
    _scopes = Bn3Deque(ScopedTaskScopeImpl) { Bn3DequeAllocator(ScopedTaskScopeImpl, Bn3Tag("scopes")) };
}

ScopedTaskScopeImpl& ScopedTaskScopeImplPool::getScope(const Bn3Tag& scope_name, std::function<bool(ScopedTaskScopeImpl&)> onStart)
{
    for (auto& scope : _scopes)
    {
        if (scope._name == scope_name)
            return scope;
    }

    auto temp_getCurrentScope = [&]() {
        return getCurrentScope();
    };

    _scopes.emplace_back(scope_name, onStart, temp_getCurrentScope);
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
void ScopedTaskScopeImplPool::clear()
{
    for (auto& scope : _scopes)
    {
        scope.stop();
    }
    _scopes.clear();
}