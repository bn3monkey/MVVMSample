#ifndef __BN3MONKEY_SCOPED_TASK_IMPL__
#define __BN3MONKEY_SCOPED_TASK_IMPL__

#include <functional>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <chrono>
#include <queue>
#include <cassert>

#include "../Log/Log.hpp"

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
    class ScopedTaskResultImpl;
    class ScopedTaskNotifier;


    class TaskLinker
    {
    public:
        void link(ScopedTaskResultImpl* result, ScopedTaskNotifier* notifier)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _valid = true;
                _result = result;
                _notifier = notifier;
            }
        }
        void unlink()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_result == nullptr && _notifier == nullptr)
                    _valid = false;
            }
        }

        void own(ScopedTaskResultImpl* result)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _result = result;
            }
        }
        void own(ScopedTaskNotifier* notifier)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _notifier = notifier;
            }
        }
        void releaseResult()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _result = nullptr;

                if (_result == nullptr && _notifier == nullptr)
                    _valid = false;
            }
        }
        void releaseNotifier()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _notifier = nullptr;

                
            }
        }

    private:
        bool _valid;
        size_t idx;

        ScopedTaskResultImpl* _result;
        ScopedTaskNotifier* _notifier;
        std::mutex _mtx;
    };

    class TaskLinkerPool
    {
    public:
        TaskLinkerPool(size_t size)
        {
            pools.resize(size);
        }
        TaskLinker* allocate(ScopedTaskResultImpl* result, ScopedTaskNotifier* notifier)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                if (_latest_deallocated)
                    return _latest_deallocated;

                
                for (size_t _new_reserved = _reserved + 1; _new_reserved != _reserved;)
                {
                    if (_new_reserved >= pools.size())
                        _new_reserved = 0;

                    auto& pool = pools[_new_reserved];
                }               
            }
        }
        void deallocate(TaskLinker* value)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                value->unlink();
                _latest_deallocated = value;
            }
        }

    private:
        TaskLinker* _latest_deallocated {nullptr};
        size_t _reserved;
        std::mutex _mtx;

        std::vector<TaskLinker> pools;
    };



    enum class ScopedTaskState
    {
        NOT_FINISHED, // 작업이 끝나기 전일 떄
        CANCELLED, // Worker가 강제로 종료되어 처리할 수 없게 되었을 때
        FINISHED, // 작업이 끝났을 때
        INVALID // 이미 기다림이 완료되었을 떄
    };

    


    class ScopedTaskResultImpl
    {
    public:
        ScopedTaskResultImpl(const char* name)
        {
            memcpy(_name, name, 256);
            LOG_D("ScopedTaskResultImpl (%s) Created (%s)", _name);
        }
        virtual ~ScopedTaskResultImpl()
        {
            if (_notifier)
            {
                LOG_D("ScopedTaskResultImpl (%s) is valid ", _name);
                _notifier->resetResult(nullptr);
            }
            LOG_D("ScopedTaskResultImpl (%s) Removed", _name);
        }
        ScopedTaskResultImpl(ScopedTaskResultImpl&& other)
        {
            memcpy(_name, other._name, 256);
            _state = other._state;

            _notifier = other._notifier;
            _notifier->resetResult(this);
            other._notifier = nullptr;
        }
        ScopedTaskResultImpl(const ScopedTaskResultImpl& other) = delete;

        void cancel()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::CANCELLED;
            }
            _cv.notify_all();
            LOG_D("Task Result cancelled (%s)", _name);
        }
        
        virtual void notify(char* result) = 0;
        virtual char* wait() = 0;
        void resetNotifier(ScopedTaskNotifier* notifier)
        {
            this->_notifier = notifier;
        }

        inline const char* name() { return _name; }
    private:
        char _name[256]{ 0 };
        ScopedTaskState _state{ ScopedTaskState::NOT_FINISHED };
        ScopedTaskNotifier* _notifier;
        // Not moved. Just use.
        std::mutex _mtx;
        std::condition_variable _cv;
    };

    template<unsigned int TypeSize>
    class ScopedTaskSizedResultImpl : public ScopedTaskResultImpl
    {
        ScopedTaskSizedResultImpl(const char* name) : ScopedTaskResultImpl(name) {}
        virtual ~ScopedTaskSizedResultImpl()
        {
           
        }
        ScopedTaskSizedResultImpl(ScopedTaskSizedResultImpl&& other) : ScopedTaskResultImpl(std::move(other))
        {
            memcpy(_result, other._result, TypeSize);
        }
        ScopedTaskSizedResultImpl(const ScopedTaskSizedResultImpl& other) = delete;
        override void notify(char* result)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::FINISHED;
                memcpy(_result, &result, TypeSize);
            }
            _cv.notify_all();
            LOG_D("Task Result notified (%s)", _name);
        }
        override char* wait() {
            if (_state == ScopedTaskState::INVALID)
            {
                LOG_E("This task (%s) is invalid!\n", _name);
                assert(false);
            }

            LOG_D("Waited by Task Result (%s)", _name);
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [&]() {
                    return _state != ScopedTaskState::NOT_FINISHED;
                    });
            }
            ResultType* ret{ nullptr };
            if (_state == ScopedTaskState::FINISHED)
                ret = (ResultType*)_result;
            _state = ScopedTaskState::INVALID;
            return ret;
        }
    private:
        char _result[TypeSize];
    };



    class ScopedTaskNotifier
    {
    public:
        ScopedTaskNotifier(const char* name, ScopedTaskResultImpl* result)
        {
            memcpy(_name, name, 256);
            _result = result;
        }
        ScopedTaskNotifier(const ScopedTaskNotifier& other) = delete;
        ScopedTaskNotifier(ScopedTaskNotifier&& other)
        {
            memcpy(_name, other._name, 256);
            
            _result = other._result;
            _result->resetNotifier(this);
            other._result = nullptr;

            LOG_D("Notifier (%s) Copied", _name);
        }

        virtual ~ScopedTaskNotifier()
        {
            if (_result)
            {
                LOG_D("This notifier is valid");
                _result->resetNotifier(nullptr);
            }
            LOG_D("Notifier (%s) Removed", _name);
        }

        void cancel()
        {
            if (_result)
                _result->cancel();
        }
        template<class ReturnType>
        void notify(const ReturnType& ret)
        {
            if (_result)
                _result->notify((char*)&ret);
        }
        void notify()
        {
            if (_result)
                _result->notify(nullptr);
        }

        void resetResult(ScopedTaskResultImpl* result)
        {
            if (_result)
                _result = result;
        }


    private:
        char _name[256]{ 0 };
        ScopedTaskResultImpl* _result;
    };

    
    class ScopedTask
    {
    public:
        explicit ScopedTask() {
            LOG_D("Scoped Task Not Initialized");
        }

        ScopedTask(const char* name)
        {
            memcpy(_name, name, 256);
            LOG_D("Scoped Task (%s) Created", _name);
        }
        
        ScopedTask(const ScopedTask& other) = delete;
        
        ScopedTask(ScopedTask&& other)
            :
            _invoke(std::move(other._invoke)),
            _notifier(std::move(other._notifier))
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            memcpy(_call_stack, other._call_stack, max_call_stack_size * 256);
            memset(other._call_stack, 0, max_call_stack_size * 256);
            
            _call_stack_size = other._call_stack_size;

            LOG_D("Scoped Task (%s) Moved", _name);
        }

        ScopedTask& operator=(ScopedTask&& other)
        {
            _invoke = std::move(other._invoke);
            _notifier = std::move(other._notifier);

            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            memcpy(_call_stack, other._call_stack, max_call_stack_size * 256);
            memset(other._call_stack, 0, max_call_stack_size * 256);

            _call_stack_size = other._call_stack_size;

            LOG_D("Scoped Task (%s) Moved", _name);
            return *this;
        }

        void clear() {
            memset(_name, 0, 256);
            memset(_call_stack, 0, max_call_stack_size * 256);
            _call_stack_size = 0;
            _invoke = nullptr;
            _notifier.unlink();
        }

        void invoke() {
            LOG_D("Scoped Task (%s) Invoked", _name);
            _invoke(_notifier);
        }
        void cancel() {
            LOG_D("Scoped Task (%s) Cancelled", _name);
            _notifier.cancel();
        }

        const char* name() const {
            return _name;
        }

        bool isInStack(const char* target_scope_name)
        {
            for (size_t i = 0; i < _call_stack_size; i++)
            {
                if (!strncmp(_call_stack[i], target_scope_name, 256))
                {
                    LOG_D("Stack (%d) has scope (%s)", i + 1, target_scope_name);
                    return true;
                }
            }
            return false;
        }
        bool addStack(ScopedTask& calling_task, const char* scope_name)
        {
            // 이 Task를 호출한 태스크의 콜스택에 이 Task가 수행되는 Scope를 추가하여 콜스택을 갱신함
 
            // 콜 스택 안에 중복된 스코프를 호출하면 안되도록 해야함.
            // 상호 대기가 될 수 있음

            auto* super_call_stack = calling_task._call_stack;
            size_t super_call_stack_size = calling_task._call_stack_size;
            
            _call_stack_size = calling_task._call_stack_size + 1;
            if (_call_stack_size > max_call_stack_size)
            {
                LOG_D("Call stack size is over than max call stack size");
                return false;
            }

            for (size_t i = 0; i < super_call_stack_size; i++)
            {
#ifdef _WIN32
                strncpy_s(_call_stack[i], super_call_stack[i], 256);
#else
                strncpy(_call_stack[i], super_call_stack[i], 256);
#endif
            }
#ifdef _WIN32
            strncpy_s(_call_stack[super_call_stack_size], scope_name, 256);
#else
            strncpy(_call_stack[super_call_stack_size], scope_name, 256);
#endif
            return true;
        }

        template<class Func, class... Args>
        auto make(Func&& func, Args&&... args) -> ScopedTaskResultImpl<decltype(func(args...))>
        {
            using ReturnType = decltype(func(args...));

            std::function<ReturnType()> onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            ScopedTaskResultImpl<ReturnType> result(_name);
            result.link(&_notifier);

            _invoke = [onTaskRunning = std::move(onTaskRunning)](ScopedTaskNotifier& notifier) mutable
            {
                invokeImpl(onTaskRunning, notifier);
            };
            return result;
        }

    private:
        
        template<class ReturnType>
        static void invokeImpl(const std::function<ReturnType()>& onTaskRunning, ScopedTaskNotifier& notifier)
        {
            auto ret = onTaskRunning();
            notifier.notify(ret);
        }
        template<>
        static void invokeImpl<void>(const std::function<void()>& onTaskRunning, ScopedTaskNotifier& notifier)
        {
            onTaskRunning();
            notifier.notify();
        }

        char _name[256]{ 0 };

        constexpr static size_t max_call_stack_size = 20;
        char _call_stack[max_call_stack_size][256]{ 0 };
        size_t _call_stack_size = 0;

        ScopedTaskNotifier _notifier;
       

        std::function<void(ScopedTaskNotifier& notifier)> _invoke;
    };

    

}


#endif // __BN3MONKEY_SCOPED_TASK_IMPL__
