#ifndef __BN3MONKEY_TASK_SCOPE_IMPL__
#define __BN3MONKEY_TASK_SCOPE_IMPL__

#include "ScopedTask.hpp"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <cassert>

#include "Log.hpp"

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

    enum class ScopeState
    {
        IDLE = 0, // �Ҵ�� �۾� ������ �����嵵 �������� ����
        EMPTY = 1, // �Ҵ�� �۾� ����
        READY = 2, // �Ҵ�� �۾� ����
        RUNNING = 3, // �۾� ����
        WAITING = 4, // 
    };

    class ScopedTaskScopeImpl
    {
    public:

        explicit ScopedTaskScopeImpl(const char* scope_name,
            std::function<void(ScopedTaskScopeImpl&)> onStart,
            std::function<void(ScopedTaskScopeImpl&)> onStop) :
            _onStart(onStart),
            _onStop(onStop)
        {
            memcpy(_name, scope_name, 256);
        }

        ScopedTaskScopeImpl(ScopedTaskScopeImpl&& other)
            : _onStart(std::move(other._onStart)),
              _onStop(std::move(other._onStop)),
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

        virtual ~ScopedTaskScopeImpl()
        {
        }

        void start()
        {
            _state = ScopeState::EMPTY;
            _thread = std::thread([&]() {worker(); });
        }
        void stop()
        {
            _state = ScopeState::IDLE;
            _id = std::thread::id();
            _cv.notify_all();
            _thread.join();
        }

        template<class Func, class... Args>
        void run(const char* task_name, Func&& func, Args&&... args)
        {
            // Scope�� ���������� ������ Ű��
            // �ڱ� �ڽ� Scope���� �ҷ����� ������ ScopeState�� Running
            // �ٸ� Scope���� �ҷ����� IDLE, EMPTY, READY, RUNNING �� �ϳ�
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (ScopeState::IDLE == _state)
                {
                    _onStart(*this);
                    // Start�� ����� ������ �������� �Ѿ.
                    // ������ ���� State�� EMPTY�� �� �����
                    if (ScopeState::EMPTY != _state)
                        LOG_E("Scope(%s) State is not empty when after starting scope.", _name);
                }
            }


            // ������ ScopeTask�� onTaskFinished�� 
            // �ƹ��͵� ����

            // ������ ScopeTask�� onTaskWaiting��
            // �ƹ��͵� ����

            // ScopeTask ���� ��û�ϱ�
            ScopedTask task{ task_name, []() {}, []() {} };
            task.make(std::forward<Func>(func), std::forward<Args>(args)...);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _tasks.push(task);
                if (_state == ScopeState::EMPTY)
                    _state = ScopeState::READY;
            }
            _cv.notify_all();
        }

        template<class Func, class... Args>
        auto call(const char* task_name, Func&& func, Args&&... args) -> ScopedTaskResultImpl<decltype(func(args...))>
        {

            auto* current_scope = getCurrentScope();
            if (compare(current_scope))
            {
                // ���� �������� �� �������� ������ �ٷ� �۵���Ű��
                ScopedTask task{ task_name, []() {}, []() {} };
                auto ret = task.make(std::forward<Func>(func), std::forward<Args>(args)...);
                task.invoke();
                return ret;
            }

            // ���� �������� �� �������� ���� ���� ���
            {
                // Scope�� ���������� ������ Ű��
                std::unique_lock<std::mutex> lock(_mtx);
                if (ScopeState::IDLE == _state)
                {
                    _onStart(*this);
                    // Start�� ����� ������ �������� �Ѿ.
                    // ������ ���� State�� EMPTY�� �� �����
                    if (ScopeState::EMPTY != _state)
                    {
                        LOG_E("Scope(%s) State is not empty when after starting scope.", _name);
                        assert(false);
                    }
                }
            }



            // ���� �������� call stack�� �� �������� ������ ��ȯ ȣ���� �ǹǷ� ���� �޼����� �Բ� �ݷ���Ű��
            {
                if (isAlreadyCalled(current_scope))
                {
                    LOG_E("current_scope is already called");
                    assert(false);
                }
            }

            // ���� �������� calling Scope�� �� �������� �ֱ�
            // �� �������� called Scope�� ���� �������� �ֱ� 
            onTaskStartedFromCallingScope(current_scope);

            // ������ ScopeTask�� onTaskFinished�� 
            // 2. �� �������� result_cv�� �˶��ֱ�
            auto onTaskFinished = [&]() {
                onTaskFinishedFromCallingScope();
            };

            // ������ ScopeTask�� onTaskWaiting��
            // 1. �� �������� result_cv���� ��ٸ���
            // 2. ���� �������� calling Scope�� �� �������� ���� �Ͱ�
            // �� �������� called Scope�� ���� �������� ����
            auto onTaskWaiting = [&]() {
                onTaskWaitingFromCallingScope();
            };

            // ScopeTask ���� ��û�ϱ�
            ScopedTask task{ task_name, onTaskWaiting, onTaskFinished };
            auto ret = task.make(std::forward<Func>(func), std::forward<Args>(args)...);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _tasks.push(task);
                if (_state == ScopeState::EMPTY)
                    _state = ScopeState::READY;
            }
            _cv.notify_all();

            return ret;
        }

        inline const char* name() { return _name; }
        inline ScopeState state() { return _state; }

        static ScopedTaskScopeImpl* getScope(const char* scope_name,
            std::function<void(ScopedTaskScopeImpl&)> onStart,
            std::function<void(ScopedTaskScopeImpl&)> onStop)
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

    private:
        // friend ScopedTaskScopeImpl& ScopedTaskScope::getScope(const char* scope_name);

    private:
        ScopedTaskScopeImpl* getCurrentScope() {
            auto id = std::this_thread::get_id();
            for (auto& iter : _scopes) {
                if (id == iter.second._id)
                    return &(iter.second);
            }
            return nullptr;
        }
        bool compare(ScopedTaskScopeImpl* other) {
            if (other == nullptr)
                return false;
            return _id == other->_id;
        }
        bool isAlreadyCalled(ScopedTaskScopeImpl* current_scope) {
            if (current_scope == nullptr)
                return false;

            FOR_DEBUG(int stack_number = 1;)
            for (auto* calling_scope = current_scope; calling_scope != nullptr; calling_scope = calling_scope->_calling_scope)
            {
                if (compare(calling_scope))
                    return true;
                FOR_DEBUG(
                    char* name = calling_scope->_name;
                    LOG_D("Stack (%d) | %s", stack_number++, name);

                    ScopeState state;
                    {
                        std::unique_lock<std::mutex> lock(_mtx);
                        state = calling_scope->_state;
                    }

                    if (state == ScopeState::RUNNING)
                    {
                        const char* task_name = calling_scope->_current_task.name();
                        LOG_D("   - Task (%s)", task_name);
                    }
                )
            }
            return false;
        }

        void onTaskStartedFromCallingScope(ScopedTaskScopeImpl* calling_scope) {

            if (calling_scope != nullptr)
            {
                LOG_D("Scope (%s) : Task Started", calling_scope->_name);
                std::unique_lock<std::mutex> lock(_mtx);
                calling_scope->_called_scope = this;
                this->_calling_scope = calling_scope;
                LOG_D("Stack Task!");
            }
            else
            {
                LOG_D("Task Started from external thread");
                LOG_D("Stack Task!");
            }
            FOR_DEBUG(
                for (auto* calling_scope = this; calling_scope != nullptr; calling_scope = calling_scope->_calling_scope)
                {
                    auto* name = calling_scope->_name;
                    LOG_D(" - Scope (%s)", name);
                }
            )
        }

        void onTaskFinishedFromCallingScope() {
            auto* name = _calling_scope->_name;
            if (_calling_scope)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_state == ScopeState::WAITING)
                {
                    LOG_D("Scope (%s) : Task Finished with notifying", name);
                    _state = ScopeState::RUNNING;
                }
                else if (_state == ScopeState::RUNNING)
                {
                    LOG_D("Scope (%s) : Task Finished without notifying", name);
                }
                else if (_state == ScopeState::IDLE)
                {
                    LOG_D("Scope (%s) : Task Cancelled", name);
                }

                
            }
            _cv.notify_all();
        }
        void onTaskWaitingFromCallingScope() {
            if (_calling_scope)
            {
                auto* name = _calling_scope->_name;
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopeState::WAITING;
                LOG_D("Scope (%s) : Task Waiting", name);

                _cv.wait(lock, [&]() {
                    return _state != ScopeState::WAITING;
                    });

                FOR_DEBUG(
                    for (auto* calling_scope = _calling_scope; calling_scope != nullptr; calling_scope = calling_scope->_calling_scope)
                    {
                        auto* name = calling_scope->_name;
                        LOG_D(" - Scope (%s)", name);
                    }
                )

                _calling_scope->_called_scope = nullptr;
                this->_calling_scope = nullptr;
                LOG_D("Unstack Task!");
            }
            else
            {
                std::unique_lock<std::mutex> lock(_mtx);

                _state = ScopeState::WAITING;
                LOG_D("No Scope : Task Waiting");
                _cv.wait(lock, [&]() {
                    return _state != ScopeState::WAITING;
                    });
            }
        }

        static std::unordered_map<std::string, ScopedTaskScopeImpl> _scopes;

        char _name[256]{ 0 };

        std::function<void(ScopedTaskScopeImpl&)> _onStart;
        std::function<void(ScopedTaskScopeImpl&)> _onStop;

        std::thread _thread;
        std::thread::id _id;

        ScopeState _state{ ScopeState::IDLE };
        ScopedTask _current_task;
        std::queue<ScopedTask> _tasks;
        std::mutex _mtx;
        std::condition_variable _cv;

        ScopedTaskScopeImpl* _calling_scope {nullptr};
        ScopedTaskScopeImpl* _called_scope{ nullptr };


        void worker()
        {
            LOG_D("Worker (%s) Start", _name);
            
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
                        _onStop(*this);
                    }

                    if (_state == ScopeState::IDLE)
                    {
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

            LOG_D("Scope (%s) : Cancel all non-executed tasks ", _name);
            while (!_tasks.empty())
            {
                _current_task = std::move(_tasks.front());
                _tasks.pop();
                _current_task.cancel();
            }
            LOG_D("Worker (%s) Ends", _name);
        }

    };
}

#endif
