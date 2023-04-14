#ifndef __BN3MONKEY_SCOPED_TASK__
#define __BN3MONKEY_SCOPED_TASK__

#include <memory>

#include "ScopedTaskImpl.hpp"
#include "ScopedTaskScopeImpl.hpp"
#include "ScopedTaskLooperImpl.hpp"


namespace Bn3Monkey
{
    template<class ReturnType>
    class ScopedTaskResult;
    class ScopedTaskScope;
    class ScopedTaskLooper;
    class ScopedTaskRunner;


    template<class ReturnType>
    class ScopedTaskResult
    {
    public: 
        ScopedTaskResult() {}
        ReturnType* wait()
        {
            if (_impl)
                return _impl->wait();
            return nullptr;
        }
        ScopedTaskResult(const ScopedTaskResult& other) = delete;
        ScopedTaskResult(ScopedTaskResult&& other) : _impl(std::move(other._impl))
        {
        }
        ScopedTaskResult& operator=(ScopedTaskResult&& other)
        {
            _impl = other._impl;
            return *this;
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
        friend class ScopedTaskLooper;

        ScopedTaskScope(const Bn3Tag& scope_name = Bn3Tag("main"));
        ScopedTaskScope(const ScopedTaskScope& other) : _impl(other._impl) {}
        

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
        ScopedTaskScopeImpl& getScope(const Bn3Tag& scope_name);
        ScopedTaskScopeImpl& _impl;
    };

    class ScopedTaskLooper
    {
    public:
        ScopedTaskLooper(const Bn3Tag& looper_name);
        
        template<class Func, class ...Args>
        void start(std::chrono::microseconds interval, const ScopedTaskScope& scope, Func&& func ,Args&&... args)
        {
            _impl.start(interval, scope._impl, std::forward<Func>(func), std::forward<Args>(args)...);
        }
        
        template<class Func, class ...Args>
        void start(std::chrono::milliseconds interval, const ScopedTaskScope& scope, Func&& func, Args&&... args)
        {
            _impl.start(interval, scope._impl, std::forward<Func>(func), std::forward<Args>(args)...);
        }

        template<class Func, class... Args>
        void start(std::chrono::seconds interval, const ScopedTaskScope& scope, Func&& func, Args&&... args)
        {
            _impl.start(interval, scope._impl, std::forward<Func>(func), std::forward<Args>(args)...);
        }

        void stop()
        {
            _impl.stop();
        }

    private:
        ScopedTaskLooperImpl& getLooper(const Bn3Tag& looper_name);
        ScopedTaskLooperImpl& _impl;
    };

    class ScopedTaskRunner
    {
    public:
        bool initialize();
        void release();
    private:
    };
}

#endif // __BN3MONKEY_SCOPED_TASK__