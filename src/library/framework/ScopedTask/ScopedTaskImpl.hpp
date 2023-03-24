#include <memory>
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
    class ScopedTaskResult;

    enum class ScopedTaskState
    {
        NOT_FINISHED, // 작업이 끝나기 전일 떄
        DISCARDED, // Worker가 강제로 종료되어 처리할 수 없게 되었을 때
        FINISHED, // 작업이 끝났을 때
    }

    template<class ResultType>
    class ScopedTaskResultImpl
    {
        friend class ScopedTaskResult;
        friend class ScopedTaskRunnerImpl;
    public:
        ScopedTaskResultImpl(
            std::function<void()> onTaskWaiting, 
            std::function<void()> onTaskFinished) 
            : _onTaskWaiting(onTaskWaiting), 
              _onTaskRestarting(onTaskRestarting),
              _onTaskFinished(onTaskFinished) {}
    private:
        void cancel() {
            state = ScopedTaskState::DISCARDED;
            _onTaskFinished();
        }
        void notify(const ResultType& result) {
            memcpy(_result, result, sizeof(ResultType));
            state = ScopedTaskState::FINISHED;
            _onTaskFinished();
        }
        ResultType* wait() {
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            _onTaskWaiting();
            ResultType* ret{nullptr};
            if (state == ScopedTaskState::FINISHED)
                ret = (ResultType *)_result; 
            state = ScopedTaskState::NOT_FINISHED;
            _onTaskRestarting();
            return ret;
        }
        
        ScopedTaskState state {ScopedTaskState::NOT_FINISHED};
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
        char _result[sizeof(ResultType)] {0}; 
    };

    template<>
    class ScopedTaskResultImpl<void>
    {
        friend class ScopedTaskResult;
        friend class ScopedTaskRunnerImpl;
    public:
       ScopedTaskResultImpl(
            std::function<void()> onTaskWaiting, 
            std::function<void()> onTaskFinished) 
            : _onTaskWaiting(onTaskWaiting), 
              _onTaskRestarting(onTaskRestarting),
              _onTaskFinished(onTaskFinished) {}
    private:
        void cancel() {
            state = ScopedTaskState::DISCARDED;
            _onTaskFinished();
        }
        void notify() {
            state = ScopedTaskState::FINISHED;
            _onTaskFinished();
        }
        void wait() {
            // wait을 호출한 Scope가 있다면, 기다리는 중임을 알린다.
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            _onTaskWaiting();
            state = ScopedTaskState::NOT_FINISHED;
            // wait을 호출한 Task가 있다면, 다시 시작했음을 알린다.
            _onTaskRestarting();
        }

        ScopedTaskState state {ScopedTaskState::NOT_FINISHED};
        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
    };


    class ScopedTask
    {
    public:
        ScopedTask(const char* name, 
            std::function<void()> onTaskWaiting, 
            std::function<void()> onTaskFinished) :
            _onTaskWaiting(onTaskWaiting), 
            _onTaskFinished(onTaskFinished)
        {
            memcpy(_name, name, 256);
        }

        ScopedTask(ScopedTask&& other)
            : _invoke(std::move(invoke)),
              _cancle(std::move(_cancel)),
              _onTaskWaiting(std::move(onTaskWaiting)),
              _onTaskFinished(std::move(onTaskFinished))
        {
            memcpy(_name, other.name, 256);
        }

        void invoke() {
            _invoke();
        }
        void cancel() {
            _cancel();
        }

       
        template<class ReturnType>
        using TaskResultReference = std::shared_ptr<ScopedTaskResultImpl<ReturnType>>;

        template<class Func, class... Args>
        auto make(Func&& func, Args&&... args) -> TaskResultReference<std::result_of_t<Func(Args...)>>
        {
             using ReturnType = std::result_of_t<Func(Args...)>;

             return makeImpl(std::forward<Func>(func), std::forward<Args>(args)..., std::is_void_v<ReturnType>);
        }  

    private:
        template<class Func, class... Args>
        auto makeImpl(Func&& func, Args&&... args, std::false_type) -> TaskResultReference<std::result_of_t<Func(Args...)>>
        {
            using ReturnType = std::result_of_t<Func(Args...)>;
            
            auto onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            std::shared_ptr<ScopedTaskResultImpl<ReturnType>> result = new TaskResultReference<ReturnType>(_onTaskWaiting, _onTaskFinished);
            std::weak_ptr<ScopedTaskResultImpl<ReturnType>> wresult = result; 
            
            _invoke = [=onTaskRunning, =wresult]() {
                auto ret = onTaskRunning();
                if (auto result = wresult.lock())
                {
                    result->notify(ret);
                }
            }

            _cancle = [=wresult]() {
                if (auto result = wresult.lock())
                {
                    result->cancel();
                }
            }

            return result;
        }

        template<class Func, class... Args>
        auto makeImpl(Func&& func, Args&&... args, std::true_type) -> TaskResultReference<std::result_of_t<Func(Args...)>>>
        {
            using ReturnType = std::result_of_t<Func(Args...)>;
            
            auto onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            std::shared_ptr<ScopedTaskResultImpl<ReturnType>> result = new TaskResultReference<ReturnType>(_onTaskWaiting, _onTaskFinished);
            std::weak_ptr<ScopedTaskResultImpl<ReturnType>> wresult = result; 
            
            _invoke = [=onTaskRunning, =wresult]() {
                onTaskRunning();
                if (auto result = wresult.lock())
                {
                    result->notify();
                }
            }

            _cancle = [=wresult]() {
                if (auto result = wresult.lock())
                {
                    result->cancel();
                }
            }

            return result;
        }

        char _name[256] {0};
        std::function<void()> _invoke;
        std::function<void()> _cancel;

        std::function<void()> _onTaskWaiting;
        std::function<void()> _onTaskFinished;
    }
}



