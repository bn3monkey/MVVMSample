#ifndef __BN3MONKEY_SCOPED_TASK_IMPL__
#define __BN3MONKEY_SCOPED_TASK_IMPL__

#include <functional>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <chrono>
#include <queue>
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
    template<class ResultType>
    class ScopedTaskResultImpl;

    template<class ReturnType>
    class ScopedTaskNotifier;

    template<class ReturnType>
    class ScopedTaskCanceller;

    template <class ResultType>
    void linkThisToResult(ScopedTaskNotifier<ResultType>* notifier, ScopedTaskResultImpl<ResultType>* result);
    template <class ResultType>
    void linkThisToNotifier(ScopedTaskResultImpl<ResultType>* result, ScopedTaskNotifier<ResultType>* notifier);
    template <class ResultType>
    void unlinkNotifier(ScopedTaskNotifier<ResultType>* notifier);


    enum class ScopedTaskState
    {
        NOT_FINISHED, // 작업이 끝나기 전일 떄
        CANCELLED, // Worker가 강제로 종료되어 처리할 수 없게 되었을 때
        FINISHED, // 작업이 끝났을 때
        INVALID // 이미 기다림이 완료되었을 떄
    };

    template<class ResultType>
    class ScopedTaskResultImpl
    {
        template <class _ResultType>
        friend void linkThisToResult(ScopedTaskNotifier<_ResultType>* notifier, ScopedTaskResultImpl<_ResultType>* result);

    public:
        ScopedTaskResultImpl(
            const char* task_name)
        {
            memcpy(_name, task_name, 256);
            LOG_V("Task Result Impl Created (%s)", _name);
        }
        virtual ~ScopedTaskResultImpl()
        {
            if (_notifier)
            {
                LOG_V("Task Result is valid (%s)", _name);
                unlinkNotifier(_notifier);
            }
            LOG_V("Task Result Impl Released (%s)", _name);
        }
        ScopedTaskResultImpl(const ScopedTaskResultImpl& other) = delete;
        ScopedTaskResultImpl(ScopedTaskResultImpl&& other)
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            memcpy(_result, other._result, sizeof(ResultType));
            memset(other._result, 0, sizeof(ResultType));

            _notifier = other._notifier;
            other._notifier = nullptr;
            linkThisToNotifier(this, _notifier);
            LOG_V("Task Result moved (%s)", _name);
        }

        void cancel()
        {
            LOG_V("Cancelled by Task Result (%s)", _name);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                state = ScopedTaskState::CANCELLED;
            }
            _cv.notify_all();
        }

        void notify(const ResultType& result) {
            LOG_V("Notified by Task Result (%s)", _name);
            memcpy(_result, &result, sizeof(ResultType));
            {
                std::unique_lock<std::mutex> lock(_mtx);
                state = ScopedTaskState::FINISHED;
            }
            _cv.notify_all();
        }

        ResultType* wait() {
            if (state == ScopedTaskState::INVALID)
            {
                LOG_E("This task (%s) is already waited!\n", _name);
                assert(false);
            }

            LOG_V("Waited by Task Result (%s)", _name);
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [&]() {
                    return state != ScopedTaskState::NOT_FINISHED;
                    });
            }
            ResultType* ret{ nullptr };
            if (state == ScopedTaskState::FINISHED)
                ret = (ResultType*)_result;
            state = ScopedTaskState::INVALID;
            return ret;
        }
        const char* name() { return _name; }

    private:
        char _name[256]{ 0 };
        ScopedTaskState state{ ScopedTaskState::NOT_FINISHED };
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
        char _result[sizeof(ResultType)]{ 0 };
        ScopedTaskNotifier<ResultType>* _notifier;

        // Not moved. Just use.
        std::mutex _mtx;
        std::condition_variable _cv;
    };

    template<>
    class ScopedTaskResultImpl<void>
    {
        template <class _ResultType>
        friend void linkThisToResult(ScopedTaskNotifier<_ResultType>* notifier, ScopedTaskResultImpl<_ResultType>* result);

    public:
        ScopedTaskResultImpl<void>(
            const char* task_name) {
            memcpy(_name, task_name, 256);
            LOG_V("Task Result Impl Created (%s)", _name);
        }
        virtual ~ScopedTaskResultImpl()
        {
            if (_notifier)
            {
                LOG_V("Task Result is valid (%s)", _name);
                unlinkNotifier(_notifier);
            }
            LOG_V("Task Result Impl Released (%s)", _name);
        }
        ScopedTaskResultImpl(const ScopedTaskResultImpl& other) = delete;
        ScopedTaskResultImpl(ScopedTaskResultImpl&& other)
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            _notifier = other._notifier;
            other._notifier = nullptr;
            linkThisToNotifier(this, _notifier);
            LOG_V("Task Result moved (%s)", _name);
        }
        void cancel()
        {
            LOG_V("Cancelled by Task Result (%s)", _name);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                state = ScopedTaskState::CANCELLED;
            }
            _cv.notify_all();
        }
        void notify() {
            LOG_V("Notified by Task Result (%s)", _name);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                state = ScopedTaskState::FINISHED;
            }
            _cv.notify_all();
        }
        void wait() {
            if (state == ScopedTaskState::INVALID)
            {
                LOG_E("This task (%s) is already waited!\n", _name);
                assert(false);
            }

            LOG_V("Waited by Task Result (%s)", _name);
            // wait을 호출한 Scope가 있다면, 기다리는 중임을 알린다.
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.

            {
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [&]() {
                    return state != ScopedTaskState::NOT_FINISHED;
                    });
            }
            state = ScopedTaskState::INVALID;          
        }
        const char* name() { return _name; }
    private:
        char _name[256]{ 0 };
        ScopedTaskState state{ ScopedTaskState::NOT_FINISHED };
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
        ScopedTaskNotifier<void>* _notifier;

        // Not moved. Just use.
        std::mutex _mtx;
        std::condition_variable _cv;
    };

    template<class ResultType>
    class ScopedTaskNotifier
    {
        template <class _ResultType>
        friend void linkThisToNotifier(ScopedTaskResultImpl<_ResultType>* result, ScopedTaskNotifier<_ResultType>* notifier);

        template <class _ResultType>
        friend void unlinkNotifier(ScopedTaskNotifier<_ResultType>* notifier);


    public:
        ScopedTaskNotifier(const char* task_name,
            ScopedTaskResultImpl<ResultType>* result) {
            memcpy(_name, task_name, 256);
            _result = result;

            linkThisToResult(this, _result);
            linkThisToNotifier(_result, this);

            LOG_V("Notifier Created from result (%s)", _name);
        }
        ScopedTaskNotifier(const ScopedTaskNotifier& other) {
            memcpy(_name, other._name, 256);
            _result = other._result;
            linkThisToResult(this, _result);

            LOG_V("Notifier Copied (%s)", _name);
        }
        ScopedTaskNotifier(ScopedTaskNotifier&& other) {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            _result = other._result;
            linkThisToResult(this, _result);

            LOG_V("Notifier Moved (%s)", _name);
        }

        void cancel() {
            LOG_V("Cancel Task (%s) by Notifier", _name);
            if (_result)
            {
                _result->cancel();
            }
        }
        void notify(const ResultType& result) {
            LOG_V("Notify Task (%s) by Notifier", _name);
            if (_result)
            {
                _result->notify(result);
            }
        }
        const char* name() { return _name; }
    private:
        ScopedTaskResultImpl<ResultType>* _result{ nullptr };
        char _name[256]{ 0 };
    };



    template<>
    class ScopedTaskNotifier<void>
    {
    public:
        template <class _ResultType>
        friend void linkThisToNotifier(ScopedTaskResultImpl<_ResultType>* result, ScopedTaskNotifier<_ResultType>* notifier);

        template <class _ResultType>
        friend void unlinkNotifier(ScopedTaskNotifier<_ResultType>* notifier);

        ScopedTaskNotifier(const char* task_name, ScopedTaskResultImpl<void>* result) {
            memcpy(_name, task_name, 256);

            _result = result;
            linkThisToResult(this, _result);
            linkThisToNotifier(_result, this);

            LOG_V("Notifier Created from result (%s)", _name);
        }
        ScopedTaskNotifier(const ScopedTaskNotifier& other) {
            memcpy(_name, other._name, 256);
            _result = other._result;
            linkThisToResult(this, _result);

            LOG_V("Notifier Copied (%s)", _name);
        }
        ScopedTaskNotifier(ScopedTaskNotifier&& other) {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            _result = other._result;
            linkThisToResult(this, _result);

            LOG_V("Notifier Moved (%s)", _name);
        }

        void cancel() {
            LOG_V("Cancel Task (%s) by Notifier", _name);
            if (_result)
            {
                _result->cancel();
            }
        }
        void notify() {
            LOG_V("Notify Task (%s) by Notifier", _name);
            if (_result)
            {
                _result->notify();
            }
        }
        const char* name() { return _name; }
    private:
        ScopedTaskResultImpl<void>* _result{ nullptr };
        char _name[256]{ 0 };
    };


    class ScopedTask
    {
    public:
        explicit ScopedTask() {
            LOG_V("Scoped Task Not Initialized");
        }

        ScopedTask(const char* name)
        {
            memcpy(_name, name, 256);
            LOG_V("Scoped Task (%s) Created", _name);
        }
        
        ScopedTask(const ScopedTask& other) :
            _invoke(other._invoke)
        {
            memcpy(_name, other._name, 256);

            memcpy(_call_stack, other._call_stack, max_call_stack_size * 256);
            _call_stack_size = other._call_stack_size;

            LOG_V("Scoped Task (%s) Copied", _name);
        }
        
        ScopedTask(ScopedTask&& other)
            :
            _invoke(std::move(other._invoke))
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            memcpy(_call_stack, other._call_stack, max_call_stack_size * 256);
            memset(other._call_stack, 0, max_call_stack_size * 256);
            
            _call_stack_size = other._call_stack_size;

            LOG_V("Scoped Task (%s) Moved", _name);
        }

        ScopedTask& operator=(ScopedTask&& other)
        {
            _invoke = std::move(other._invoke);
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            memcpy(_call_stack, other._call_stack, max_call_stack_size * 256);
            memset(other._call_stack, 0, max_call_stack_size * 256);

            _call_stack_size = other._call_stack_size;

            LOG_V("Scoped Task (%s) Moved", _name);
            return *this;
        }

        void invoke() {
            _invoke(true);
        }
        void cancel() {
            _invoke(false);
        }

        const char* name() {
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
            std::function<void()> _onTaskWaiting = []() {};
            std::function<void()> _onTaskFinished = []() {};
            ScopedTaskResultImpl<ReturnType> result(_name);
            ScopedTaskNotifier<ReturnType> notifier{ _name,  &result };

            _invoke = [onTaskRunning = std::move(onTaskRunning), notifier](bool finished) mutable
            {
                if (finished)
                    invokeImpl(onTaskRunning, notifier);
                else
                    cancelImpl(notifier);
            };
            return result;
        }

    private:
        
        template<class ReturnType>
        static void invokeImpl(const std::function<ReturnType()>& onTaskRunning, ScopedTaskNotifier<ReturnType>& notifier)
        {
            auto ret = onTaskRunning();
            notifier.notify(ret);
        }
        template<>
        static void invokeImpl<void>(const std::function<void()>& onTaskRunning, ScopedTaskNotifier<void>& notifier)
        {
            onTaskRunning();
            notifier.notify();
        }

        template<class ReturnType>
        static void cancelImpl(ScopedTaskNotifier<ReturnType>& notifier)
        {
            notifier.cancel();
        }

        char _name[256]{ 0 };

        constexpr static size_t max_call_stack_size = 20;
        char _call_stack[max_call_stack_size][256]{ 0 };
        size_t _call_stack_size = 0;
        

        std::function<void(bool)> _invoke;
    };


    template <class ResultType>
    void linkThisToResult(ScopedTaskNotifier<ResultType>* notifier, ScopedTaskResultImpl<ResultType>* result)
    {        
        if (result)
        {
            LOG_D("Link Notifier (%s) To Result (%s)", notifier->name(), result->name());
            result->_notifier = notifier;
        }
        else
        {
            LOG_D("Result is null");
        }
    }
    template <class ResultType>
    void linkThisToNotifier(ScopedTaskResultImpl<ResultType>* result, ScopedTaskNotifier<ResultType>* notifier)
    {
        if (notifier)
        {
            LOG_D("Link Notifier (%s) To Result (%s)", notifier->name(), result->name());
            notifier->_result = result;
        }
        else
        {
            LOG_D("Notifier is null");
        }
    }
    template <class ResultType>
    void unlinkNotifier(ScopedTaskNotifier<ResultType>* notifier)
    {
        if (notifier)
        {
            LOG_D("unlink Notifier", notifier->name());
            notifier->_result = nullptr;
        }
        else
        {
            LOG_D("Notifier is null");
        }
    }

}


#endif // __BN3MONKEY_SCOPED_TASK_IMPL__
