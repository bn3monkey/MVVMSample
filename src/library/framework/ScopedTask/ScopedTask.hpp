#ifndef __BN3MONKEY_SCOPED_TASK__
#define __BN3MONKEY_SCOPED_TASK__

#include <memory>

#include "ScopedTaskImpl.hpp"
#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskRunnerImpl.hpp"


namespace Bn3Monkey
{
    template<class ReturnType>
    class ScopedTaskResult;
    class ScopedTaskScope;
    class ScopedTaskRunner;


    template<class ReturnType>
    class ScopedTaskResult
    {
    public: 
        ReturnType* wait()
        {
            return _impl->wait();
        }
        ScopedTaskResult(const ScopedTaskResult& other) = delete;
        ScopedTaskResult(ScopedTaskResult&& other) : _impl(std::move(other._impl))
        {
        }

    private:
        friend class ScopedTaskScope;

        ScopedTaskResult(std::shared_ptr<ScopedTaskResultImpl<ReturnType>> impl) : _impl(std::move(impl))
        {
        }
        std::shared_ptr<ScopedTaskResultImpl<ReturnType>> _impl;
    };

    class ScopedTaskScope
    {
    public:
        ScopedTaskScope(const char* scope_name);

        template<class Func, class... Args>
        void run(const Bn3Tag& task_name, Func&& func, Args&&... args)
        {
            _impl.run(task_name, std::forward<Func>(func), std::forward<Args>(args)...);
        }

        template<class Func, class... Args>
        auto call(const Bn3Tag& task_name, Func&& func, Args&&... args) -> ScopedTaskResult<std::result_of_t<Func(Args...)>>
        {
            auto _result = _impl.call(task_name, std::forward<Func>(func), std::forward<Args>(args)...);
            
            //return ScopedTaskResult<std::result_of_t<Func(Args...)>>();
            return ScopedTaskResult(std::move(_result));
        }


    private:
        ScopedTaskScopeImpl& getScope(const char* scope_name);
        ScopedTaskScopeImpl& _impl;
    };

    /*
    class ScopedTaskLooper
    {
    public:
        ScopedTaskLooper(const ScopedTaskScope& scope, std::function<void()> task);
        void start();
        void stop();

    private:
        ScopedTaskLooperImpl _impl;
    };
    */

    class ScopedTaskRunner
    {
    public:
        bool initialize();
        void release();
    private:
    };
}

#endif // __BN3MONKEY_SCOPED_TASK__