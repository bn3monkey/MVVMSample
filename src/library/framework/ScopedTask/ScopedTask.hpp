#ifndef __SCOPED_TASK_RUNNER__
#define __SCOPED_TASK_RUNNER__

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
            return _impl.wait();
        }
    private:
        friend class ScopedTaskScope;
        ScopedTaskResult(ScopedTaskResultImpl&& impl)
        {
            _impl = impl;
        }
        ScopedTaskResultImpl _impl; 
    };

    class ScopedTaskScope
    {
    public:
        ScopedTaskScope(const char* scope_name);

        template<class Func, class... Args>
        void run(const char* task_name, Func&& func, Args&&... args)
        {
            _impl.run(task_name, std::forward<Func>(func), std::forward<Args>(args)...);
        }

        template<class Func, class... Args>
        ScopedTask call(const char* task_name, Func&& func, Args&&... args)
        {
            auto _result = _impl.call(task_name, std::forward<Func>(func), std::forward<Args>(args)...);
            return ScopedTaskResult(_result);
        }

    private:
        ScopedTaskScopeImpl& getScope(const char* scope_name);
        ScopedTaskScopeImpl& _impl;
    };

    class ScopedTaskRunner
    {
    public:
        bool initialize();
        void release();
    private:
    };
}

#endif