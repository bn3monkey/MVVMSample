#include "TaskScopeImpl.hpp"
#include "ScopedThreadPoolHelper.hpp"

static const char* getScopeStatusString(Bn3Monkey::TaskScopeImpl::ScopeStatus status)
{
    using namespace Bn3Monkey;
    switch (status)
    {
    case TaskScopeImpl::ScopeStatus::IDLE: return "IDLE";
    case TaskScopeImpl::ScopeStatus::EMPTY: return "EMPTY";
    case TaskScopeImpl::ScopeStatus::READY: return "READY";
    case TaskScopeImpl::ScopeStatus::WORKING: return "WORKING";
    case TaskScopeImpl::ScopeStatus::WAITING: return "WAITING";
    }
}

Bn3Monkey::TaskScopeImpl::TaskScopeImpl(const char* name, std::function<void()> onTimeout) : _name(name),  _status(ScopeStatus::IDLE), _onTimeout(onTimeout)
{
    _worker = std::thread(worker);
}
Bn3Monkey::TaskScopeImpl::~TaskScopeImpl()
{
    is_running = false;
    _worker.join();
}
Bn3Monkey::TaskScopeImpl::ScopeStatus Bn3Monkey::TaskScopeImpl::getStatus() {
    TaskScopeImpl::ScopeStatus temp;
    if (!_task_mtx.try_lock())
        return _status;
    else
    {
        std::unique_lock<std::mutex> lock(_task_mtx);
        temp = _status;
    }
    return temp;
}

void Bn3Monkey::TaskScopeImpl::run(const char* name, std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(_task_mtx);
        if (_status == ScopeStatus::IDLE)
        {
            LOG_V("Scope %s starts", _name);
            // Task Scope를 다시 활성화하고 큐의 상태에 따라 Task Scope의 상태를 결정한다.
            if (_tasks.empty())
                _status = ScopeStatus::EMPTY;
            else
                _status = ScopeStatus::READY;
            LOG_V("Scope %s Status is %s", _name, getScopeStatusString(_status));
            _worker = std::thread(worker);
        }

        // Task를 넣는다.
        NamedTask namedTask;
        strncpy(namedTask.name, name);
        namedTask.task = task;

        _tasks.push(namedTask);

        // Task Scope는 이제 대기 중이다.
        if (_status == ScopeStatus::EMPTY) {
            _status = ScopeStatus::READY;
            LOG_V("Scope %s Status is %s", _name, getScopeStatusString(_status));
        }
    }
    _task_cv.notify_one();
}

void call(const char* task_name, TaskScopeImpl& called_scope, std::function<void()> task)
{
    // 이 Task를 호출한 TaskScope와 대상이 되는 TaskScope를 비교한다.
            // 같으면, 여기서 수행한다.

            // 다르면, 대상이 되는 TaskScope가 IDLE, EMPTY, READY, RUNNING이면 바로 수행한다.
                // TaskScope에 Run 요청을 보낸다.
                // TaskScopeImpl에 Enqueue한다.
    if (other.)

    {
        std::unique_lock<std::mutex> lock(_task_mtx);
        if (_status == ScopeStatus::IDLE)
        {
            LOG_V("Scope %s starts", _name);
            // Task Scope를 다시 활성화하고 큐의 상태에 따라 Task Scope의 상태를 결정한다.
            if (_tasks.empty())
                _status = ScopeStatus::EMPTY;
            else
                _status = ScopeStatus::READY;
            LOG_V("Scope %s Status is %s", _name, getScopeStatusString(_status));
            _worker = std::thread(worker);
        }

        // Task를 넣는다.
        NamedTask namedTask;
        strncpy(namedTask.name, name);
        namedTask.task = task;

        _tasks.push(namedTask);

        // Task Scope는 이제 대기 중이다.
        if (_status == ScopeStatus::EMPTY) {
            _status = ScopeStatus::READY;
            LOG_V("Scope %s Status is %s", _name, getScopeStatusString(_status));
        }
    }
    _task_cv.notify_one();

    
            // 대상이 되는 TaskScope가  또는 WAITING이다. 
            // 상위에 호출한 Task 중 대상이 되는 TaskScope가 있을 경우, 런타임 오류를 내뱉는다.
            
            // 이 Task를 호출한 TaskScope의 상태를 WAITING으로 바꾼다.
}

void Bn3Monkey::TaskScopeImpl::stop()
{
    LOG_V("Scope %s is stopped", _name);
    is_running = false;
    _worker.join();
}

void Bn3Monkey::TaskScopeImpl::worker()
{
    // 현재 Thread ID를 따로 배정한다.
    _worker_id = std::this_thread::get_id();

    // 디버깅을 위해 Thread의 이름을 설정한다.
    std::string thread_name{ "Worker (" };
    thread_name += _name;
    thread_name += ")";
    setThreadName(thread_name.c_str());

    is_running = true;
    
    for (;;)
    {
        
        LOG_V("Worker (%s) Starts", _name);

        NamedTask task;
        {
            std::unique_lock<std::mutex> lock(_task_mtx);
            bool is_not_waiting = _task_cv.wait_for(lock, timeout, [&] {
                return !is_running || !_tasks.empty();
                });

            // 쓰레드가 너무 오랜 시간동안 대기하면 어플리케이션의 쓰레드 목록에서 내린다. 
            if (!is_not_waiting)
            {
                LOG_D("Worker %s : Timeout", _name);
                is_running = false;
                if (_onTimeout)
                    _onTimeout(*this);
            }
            // 쓰레드를 더 이상 동작시키지 않으면 쓰레드 목록에서 내린다.
            if (!is_running)
            {
                _status = ScopeStatus::IDLE;
                
                LOG_D("Scope %s Status is %s", __name, getScopeStatusString(_status));
                break;
            }

            if (_status == ScopeStatus::READY)
            {
                LOG_D("Scope %s Status is %s", _name, getScopeStatusString(_status));
                _status = ScopeStatus::WORKING;
            }
            else
            {
                LOG_D("Worker %s has state error <Current Scope Status : %s>)", _name, getScopeStatusString(_status));
            }
            task = std::move(_tasks.front());
            _tasks.pop();
        }

        strncpy(task_name, task.name, 256);
        task.task();

        {
            std::unique_lock<std::mutex> lock(_task_mtx);
            if (_status == ScopeStatus::WORKING)
            {
                if (_tasks.empty())
                {
                    LOG_D("Scope %s Status is %s", _name, getScopeStatusString(_status));
                    _status = ScopeStatus::EMPTY;
                }
                else
                {
                    LOG_D("Scope %s Status is %s", _name, getScopeStatusString(_status));
                    _status = ScopeStatus::READY;
                }
            }
            else
            {
                LOG_D("Worker %s has state error <Current Scope Status : %s>)", _name, getScopeStatusString(_status));
            }
        }
    }

    LOG_V("Worker (%s) Ends", _name);
}
