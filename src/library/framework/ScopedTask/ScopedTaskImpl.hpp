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


#include "../MemoryPool/MemoryPool.hpp"

#ifdef __BN3MONKEY_MEMORY_POOL__
#define ALLOC(TYPE, ...) Bn3Monkey::Bn3MemoryPool::allocate_and_initialize<TYPE>(__VA_ARGS__)
#define MAKE_SHARED(TYPE, PTR) std::shared_ptr<TYPE>(PTR, [](TYPE* ptr) { Bn3Monkey::Bn3MemoryPool::deallocate(ptr); })
#else
#define ALLOC(TYPE, ...) new TYPE(__VA_ARGS__)
#define MAKE_SHARED(TYPE, PTR) std::shared_ptr<TYPE>(PTR)
#endif

namespace Bn3Monkey
{
    enum class ScopedTaskState
    {
        NOT_FINISHED, // 작업이 끝나기 전일 떄
        CANCELLED, // Worker가 강제로 종료되어 처리할 수 없게 되었을 때
        FINISHED, // 작업이 끝났을 때
        INVALID // 이미 기다림이 완료되었을 떄
    };

    

    template<class Type>
    class ScopedTaskResultImpl
    {
    public:
        ScopedTaskResultImpl(const char* name)
        {
            memcpy(_name, name, 256);
            LOG_D("ScopedTaskResultImpl (%s) Created", _name);
        }
        virtual ~ScopedTaskResultImpl()
        {
            LOG_D("ScopedTaskResultImpl (%s) Removed", _name);
        }
        ScopedTaskResultImpl(ScopedTaskResultImpl&& other) = delete;
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
        
        virtual void notify(const Type& result)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::FINISHED;
                memcpy(_result, &result, sizeof(Type));
            }
            _cv.notify_all();
            LOG_D("Task Result notified (%s)", _name);
        }
        virtual Type* wait()
        {
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
            
            Type* ret{ nullptr };
            if (_state == ScopedTaskState::FINISHED)
                ret = (Type*)_result;
            _state = ScopedTaskState::INVALID;
            return ret;
        }


        inline const char* name() { return _name; }
    private:
        char _name[256]{ 0 };
        ScopedTaskState _state{ ScopedTaskState::NOT_FINISHED };
        char _result[sizeof(Type)]{ 0 };
        // Not moved. Just use.
        std::mutex _mtx;
        std::condition_variable _cv;
    };

    template<>
    class ScopedTaskResultImpl<void>
    {
    public:
        ScopedTaskResultImpl(const char* name)
        {
            memcpy(_name, name, 256);
            LOG_D("ScopedTaskResultImpl (%s) Created", _name);
        }
        virtual ~ScopedTaskResultImpl()
        {
            LOG_D("ScopedTaskResultImpl (%s) Removed", _name);
        }
        ScopedTaskResultImpl(ScopedTaskResultImpl&& other) = delete;
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

        void notify()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::FINISHED;
            }
            _cv.notify_all();
            LOG_D("Task Result notified (%s)", _name);
        }
        void wait()
        {
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

            _state = ScopedTaskState::INVALID;
        }


        inline const char* name() { return _name; }
    private:
        char _name[256]{ 0 };
        ScopedTaskState _state{ ScopedTaskState::NOT_FINISHED };

        // Not moved. Just use.
        std::mutex _mtx;
        std::condition_variable _cv;
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
            _invoke(std::move(other._invoke))
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
        }

        void invoke() {
            LOG_D("Scoped Task (%s) Invoked", _name);
            _invoke(true);
        }
        void cancel() {
            LOG_D("Scoped Task (%s) Cancelled", _name);
            _invoke(false);
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
        auto make(Func&& func, Args&&... args) -> std::shared_ptr<ScopedTaskResultImpl<decltype(func(args...))>>
        {
            using ReturnType = decltype(func(args...));

            std::function<ReturnType()> onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

            auto* raw_ptr = ALLOC(ScopedTaskResultImpl<ReturnType>, _name);
            auto result = MAKE_SHARED(ScopedTaskResultImpl<ReturnType>, raw_ptr);
            
            /*
            auto* raw_ptr = Bn3Monkey::Bn3MemoryPool::allocate_and_initialize<ScopedTaskResultImpl<ReturnType>>(_name);
            auto result = std::shared_ptr<ScopedTaskResultImpl<ReturnType>>(raw_ptr, [](ScopedTaskResultImpl<ReturnType>* ptr)
                {
                    Bn3Monkey::Bn3MemoryPool::deallocate(ptr);
                });
                */

            auto wresult = std::weak_ptr<ScopedTaskResultImpl<ReturnType>>(result);
           
            _invoke = [onTaskRunning = std::move(onTaskRunning), wresult = wresult](bool value) mutable
            {
                if (value)
                    invokeImpl(onTaskRunning, wresult);
                else
                {
                    cancelImpl(wresult);
                }
            };
            return result;
        }

    private:
        
        template<class ReturnType>
        static void invokeImpl(const std::function<ReturnType()>& onTaskRunning, std::weak_ptr<ScopedTaskResultImpl<ReturnType>> wresult)
        {
            auto ret = onTaskRunning();
            if (auto result = wresult.lock())
                result->notify(ret);
        }
        template<>
        static void invokeImpl<void>(const std::function<void()>& onTaskRunning, std::weak_ptr<ScopedTaskResultImpl<void>> wresult)
        {
            onTaskRunning();
            if (auto result = wresult.lock())
                result->notify();
        }

        template<class ReturnType>
        static void cancelImpl(std::weak_ptr<ScopedTaskResultImpl<ReturnType>> wresult)
        {
            if (auto result = wresult.lock())
                result->cancel();
        }

        char _name[256]{ 0 };

        constexpr static size_t max_call_stack_size = 20;
        char _call_stack[max_call_stack_size][256]{ 0 };
        size_t _call_stack_size = 0;
              

        std::function<void(bool)> _invoke;
    };

    

}


#endif // __BN3MONKEY_SCOPED_TASK_IMPL__
