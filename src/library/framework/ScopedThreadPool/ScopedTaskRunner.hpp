#ifndef __SCOPED_TASK_RUNNER__
#define __SCOPED_TASK_RUNNER__

#include "TaskScopeImpl.hpp"
#include "TaskRunnerImpl.hpp"
#include "TaskResultImpl.hpp"

#include <memory>


namespace Bn3Monkey
{        
    class TaskScope;
    class ScopedThreadPool;

    /* Call을 통해 호출할 경우, 디버깅을 위해 반드시 Call Stack을 만들어야 한다! */

    class TaskScope
    {
    public:
        static TaskScope main;
        TaskScope(const char* name);
    
    private:
        std::weak_ptr<TaskScopeImpl> _impl;
    }

    template <class ReturnType>
    class TaskResult
    {
    public:
        ReturnType get() {

        }
    private:
    }
    template<>
    class TaskResult<void>
    {
    public:
        void get() {

        }
    }


    class TaskRunner
    {
    public:
        void initialize();
        void release();

        template<class F, class Args..>
        void run(TaskScope scope, F&& f, Args&& args)
        {

        }

        template<class F, class Args...>
        TaskResult<std::result_of<F(Args...)>::type> call(TaskScope scope, F&& f, Args&& args)
        {
            using ReturnType = std::result_of<F(Args...)>::type;
        }

    private:
        TaskRunnerImpl _impl;
    };
}

#endif