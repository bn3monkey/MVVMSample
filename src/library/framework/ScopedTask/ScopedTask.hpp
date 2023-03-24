#ifndef __SCOPED_TASK_RUNNER__
#define __SCOPED_TASK_RUNNER__

#include <memory>

#include "ScopedTaskImpl.hpp"
#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskRunnerImpl.hpp"


namespace Bn3Monkey
{
    
    /* Call을 통해 호출할 경우, 디버깅을 위해 반드시 Call Stack을 만들어야 한다! */

    template<class ReturnType>
    class ScopedTaskResult
    {
    public:
        // ScopedTaskRunner 
        ReturnType* wait();
    private:
        std::shared_ptr<ScopedTaskResultImpl> _impl; 
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
            return _impl.call(task_name, std::forward<Func>(func), std::forward<Args>(args)...);
        }

    private:
        ScopedTaskImpl& getScope(const char* scope_name);
        ScopedTaskImpl& _impl;
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