#ifndef __BN3MONKEY_SCOPED_TASK_IMPL__
#define __BN3MONKEY_SCOPED_TASK_IMPL__

#include <functional>
#include <mutex>
#include <condition_variable>
#include <type_traits>

#include "../Log/Log.hpp"

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

    template <class ResultType>
    void linkThisToResult(ScopedTaskNotifier<ResultType>* notifier, ScopedTaskResultImpl<ResultType>* result);
    template <class ResultType>
    void linkThisToNotifier(ScopedTaskResultImpl<ResultType>* result, ScopedTaskNotifier<ResultType>* notifier);
    template <class ResultType>
    void unlinkNotifier(ScopedTaskNotifier<ResultType>* notifier);


    enum class ScopedTaskState
    {
        NOT_FINISHED, // 작업이 끝나기 전일 떄
        DISCARDED, // Worker가 강제로 종료되어 처리할 수 없게 되었을 때
        FINISHED, // 작업이 끝났을 때
    };

    template<class ResultType>
    class ScopedTaskResultImpl
    {
        template <class _ResultType>
        friend void linkThisToResult(ScopedTaskNotifier<_ResultType>* notifier, ScopedTaskResultImpl<_ResultType>* result);

    public:
        ScopedTaskResultImpl(
            const char* task_name,
            std::function<void()> onTaskWaiting,
            std::function<void()> onTaskFinished)
            : _onTaskWaiting(onTaskWaiting),
            _onTaskFinished(onTaskFinished)
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
            : _onTaskWaiting(std::move(other._onTaskWaiting)),
            _onTaskFinished(std::move(other._onTaskFinished))
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

        void cancel() {
            LOG_V("Cancelled by Task Result (%s)", _name);
            state = ScopedTaskState::DISCARDED;
            _onTaskFinished();
        }
        void notify(const ResultType& result) {
            LOG_V("Notified by Task Result (%s)", _name);
            memcpy(_result, &result, sizeof(ResultType));
            state = ScopedTaskState::FINISHED;
            _onTaskFinished();
        }
        ResultType* wait() {
            LOG_V("Waited by Task Result (%s)", _name);
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            _onTaskWaiting();
            ResultType* ret{ nullptr };
            if (state == ScopedTaskState::FINISHED)
                ret = (ResultType*)_result;
            return ret;
        }

    private:
        char _name[256]{ 0 };
        ScopedTaskState state{ ScopedTaskState::NOT_FINISHED };
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
        char _result[sizeof(ResultType)]{ 0 };
        ScopedTaskNotifier<ResultType>* _notifier;
    };

    template<>
    class ScopedTaskResultImpl<void>
    {
        template <class _ResultType>
        friend void linkThisToResult(ScopedTaskNotifier<_ResultType>* notifier, ScopedTaskResultImpl<_ResultType>* result);

    public:
        ScopedTaskResultImpl<void>(
            const char* task_name,
            std::function<void()> onTaskWaiting,
            std::function<void()> onTaskFinished)
            : _onTaskWaiting(onTaskWaiting),
            _onTaskFinished(onTaskFinished) {
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
            : _onTaskWaiting(std::move(other._onTaskWaiting)),
            _onTaskFinished(std::move(other._onTaskFinished))
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);

            _notifier = other._notifier;
            other._notifier = nullptr;
            linkThisToNotifier(this, _notifier);
            LOG_V("Task Result moved (%s)", _name);
        }
        void cancel() {
            LOG_V("Cancelled by Task Result (%s)", _name);
            state = ScopedTaskState::DISCARDED;
            _onTaskFinished();
        }
        void notify() {
            LOG_V("Notified by Task Result (%s)", _name);
            state = ScopedTaskState::FINISHED;
            _onTaskFinished();
        }
        void wait() {
            LOG_V("Waited by Task Result (%s)", _name);
            // wait을 호출한 Scope가 있다면, 기다리는 중임을 알린다.
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            _onTaskWaiting();
        }
    private:
        char _name[256]{ 0 };
        ScopedTaskState state{ ScopedTaskState::NOT_FINISHED };
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
        ScopedTaskNotifier<void>* _notifier;
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
        ScopedTask(const char* name,
            std::function<void()> onTaskWaiting,
            std::function<void()> onTaskFinished) :
            _onTaskWaiting(onTaskWaiting),
            _onTaskFinished(onTaskFinished)
        {
            memcpy(_name, name, 256);
            LOG_V("Scoped Task (%s) Created", _name);
        }

        ScopedTask(ScopedTask&& other)
            :
            _notifier(std::move(other._notifier)),
            _invoke(std::move(other._invoke)),
            _cancel(std::move(other._cancel)),
            _onTaskWaiting(std::move(other._onTaskWaiting)),
            _onTaskFinished(std::move(other._onTaskFinished))
        {
            memcpy(_name, other._name, 256);
            memset(other._name, 0, 256);
            LOG_V("Scoped Task (%s) Moved", _name);
        }

        void invoke() {
            _invoke();
        }
        void cancel() {
            _cancel();
        }
        const char* name() {
            return _name;
        }



        template<class Func, class... Args>
        auto make(Func&& func, Args&&... args) -> ScopedTaskResultImpl<decltype(func(args...))>
        {
            using ReturnType = decltype(func(args...));

            std::function<ReturnType()> onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            ScopedTaskResultImpl<ReturnType> result(_name, _onTaskWaiting, _onTaskFinished);
            ScopedTaskNotifier<ReturnType> notifier{ _name,  &result };

            _notifier = [&, notifier]() mutable {
                _invoke = [onTaskRunning = std::move(onTaskRunning), &notifier]() {
                    invokeImpl(onTaskRunning, notifier);
                };
                _cancel = [this, &notifier]() {
                    cancelImpl(notifier);
                };
            };
            _notifier();

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
        std::function<void()> _notifier;

        std::function<void()> _invoke;
        std::function<void()> _cancel;

        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
    };


    template <class ResultType>
    void linkThisToResult(ScopedTaskNotifier<ResultType>* notifier, ScopedTaskResultImpl<ResultType>* result)
    {
        result->_notifier = notifier;
    }
    template <class ResultType>
    void linkThisToNotifier(ScopedTaskResultImpl<ResultType>* result, ScopedTaskNotifier<ResultType>* notifier)
    {
        notifier->_result = result;
    }
    template <class ResultType>
    void unlinkNotifier(ScopedTaskNotifier<ResultType>* notifier)
    {
        notifier->_result = nullptr;
    }

}


#endif // __BN3MONKEY_SCOPED_TASK_IMPL__
