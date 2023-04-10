#ifndef __BN3MONKEY_SCOPED_TASK_IMPL__
#define __BN3MONKEY_SCOPED_TASK_IMPL__

#include <functional>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <chrono>
#include <queue>
#include <cassert>


#ifdef BN3MONKEY_DEBUG
#define FOR_DEBUG(t) t
#else 
#define FOR_DEBUG(t)
#endif

#include "../Log/Log.hpp"

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
#define MAKE_SHARED(TYPE, TAG, ...) Bn3Monkey::makeSharedFromMemoryPool<TYPE>(TAG, __VA_ARGS__)
#define Bn3Queue(TYPE) Bn3Monkey::Bn3Container::queue<TYPE>
#define Bn3Map(KEY, VALUE) Bn3Monkey::Bn3Container::map<KEY, VALUE>
#define Bn3String() Bn3Monkey::Bn3Container::string
#define Bn3Vector(TYPE) Bn3Monkey::Bn3Container::vector<TYPE>
#define Bn3Deque(TYPE) Bn3Monkey::Bn3Container::deque<TYPE>

#define Bn3QueueAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3MapAllocator(KEY, VALUE, TAG) Bn3Monkey::Bn3Allocator<std::pair<const KEY, VALUE>>(TAG)
#define Bn3StringAllocator(TAG) Bn3Monkey::Bn3Allocator<char>(TAG)
#define Bn3VectorAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3DequeAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)

#else
#define MAKE_SHARED(TYPE, TAG, ...) std::shared_ptr<TYPE>(new TYPE(__VA_ARGS__))
#define Bn3Queue(TYPE, TAG) std::queue<TYPE>
#define Bn3Map(KEY, VALUE, TAG) std::unordered_map<KEY, VALUE>
#define Bn3String(TAG) std::string
#define Bn3Vector(TYPE, TAG) std::vector<TYPE>
#define Bn3Deque(TYPE) std::deque<TYPE>

#define Bn3QueueAllocator(TYPE, TAG) 
#define Bn3MapAllocator(KEY, VALUE, TAG) 
#define Bn3StringAllocator(TAG) 
#define Bn3VectorAllocator(TYPE, TAG) 
#define Bn3DequeAllocator(TYPE, TAG)
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
        ScopedTaskResultImpl(const Bn3Tag& task_name)
        {
            _name = task_name;
            LOG_D("ScopedTaskResultImpl (%s) Created", _name.str());
        }
        virtual ~ScopedTaskResultImpl()
        {
            LOG_D("ScopedTaskResultImpl (%s) Removed", _name.str());
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
            LOG_D("Task Result cancelled (%s)", _name.str());
        }
        
        virtual void notify(const Type& result)
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::FINISHED;
                memcpy(_result, &result, sizeof(Type));
            }
            _cv.notify_all();
            LOG_D("Task Result notified (%s)", _name.str());
        }
        virtual Type* wait()
        {
            if (_state == ScopedTaskState::INVALID)
            {
                LOG_E("This task (%s) is invalid!\n", _name.str());
                assert(false);
            }

            LOG_D("Waited by Task Result (%s)", _name.str());
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


        inline const char* name() { return _name.str(); }
    private:
        Bn3Tag _name;
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
        ScopedTaskResultImpl(const Bn3Tag& task_name)
        {
            _name = task_name;
            LOG_D("ScopedTaskResultImpl (%s) Created", _name.str());
        }
        virtual ~ScopedTaskResultImpl()
        {
            LOG_D("ScopedTaskResultImpl (%s) Removed", _name.str());
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
            LOG_D("Task Result cancelled (%s)", _name.str());
        }

        void notify()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _state = ScopedTaskState::FINISHED;
            }
            _cv.notify_all();
            LOG_D("Task Result notified (%s)", _name.str());
        }
        void wait()
        {
            if (_state == ScopedTaskState::INVALID)
            {
                LOG_E("This task (%s) is invalid!\n", _name.str());
                assert(false);
            }

            LOG_D("Waited by Task Result (%s)", _name.str());
            // 처리하고 있는 Scope가 처리가 끝나면 알려준다.
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [&]() {
                    return _state != ScopedTaskState::NOT_FINISHED;
                    });
            }

            _state = ScopedTaskState::INVALID;
        }


        inline const char* name() { return _name.str(); }
    private:
        Bn3Tag _name;
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

        ScopedTask(const Bn3Tag& name)
        {
            _name = name;
            _call_stack = Bn3Vector(Bn3Tag) { Bn3VectorAllocator(Bn3Tag, Bn3Tag("stack_", name)) };
            LOG_D("Scoped Task (%s) Created", _name.str());
        }
        
        virtual ~ScopedTask()
        {
        }

        ScopedTask(const ScopedTask& other) = delete;
        
        ScopedTask(ScopedTask&& other)
            :
            _name(other._name),
            _invoke(std::move(other._invoke)),
            _call_stack(std::move(other._call_stack))
        {            
            LOG_D("Scoped Task (%s) Moved", _name.str());
        }

        ScopedTask& operator=(ScopedTask&& other)
        {
            _name = std::move(other._name);
            _invoke = std::move(other._invoke);
            _call_stack = std::move(other._call_stack);
            
            LOG_D("Scoped Task (%s) Moved", _name.str());
            return *this;
        }

        void clear() {
            _name.clear();
            _invoke = nullptr;
            _call_stack.clear();
        }

        void invoke() {
            LOG_D("Scoped Task (%s) Invoked", _name.str());
            _invoke(true);
        }
        void cancel() {
            LOG_D("Scoped Task (%s) Cancelled", _name.str());
            _invoke(false);
        }

        const char* name() const {
            return _name.str();
        }

        bool isInStack(const Bn3Tag& target_scope_name)
        {
            for (auto& scope_name : _call_stack)
            {
                if (scope_name == target_scope_name)
                {
                    LOG_D("Task (%s) call stack has scope (%s)", _name.str(), target_scope_name.str());
                    return true;
                }
            }
            return false;
        }
        void addStack(ScopedTask& calling_task, const Bn3Tag& scope_name)
        {
            // 이 Task를 호출한 태스크의 콜스택에 이 Task가 수행되는 Scope를 추가하여 콜스택을 갱신함
 
            // 콜 스택 안에 중복된 스코프를 호출하면 안되도록 해야함.
            // 상호 대기가 될 수 있음

            auto& super_call_stack = calling_task._call_stack;
            _call_stack.insert(_call_stack.end(), super_call_stack.begin(), super_call_stack.end());
            _call_stack.push_back(scope_name);
        }

        template<class Func, class... Args>
        auto make(Func&& func, Args&&... args) -> std::shared_ptr<ScopedTaskResultImpl<decltype(func(args...))>>
        {
            using ReturnType = decltype(func(args...));

            std::function<ReturnType()> onTaskRunning = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

            auto result = MAKE_SHARED(ScopedTaskResultImpl<ReturnType>, _name, _name);
            if (!result)
            {
                LOG_E("Cannot make task result from task (%s)", _name.str());
                return result;
            }

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

        Bn3Tag _name;
                
        Bn3Vector(Bn3Tag) _call_stack;

        std::function<void(bool)> _invoke;
    };

    

}


#endif // __BN3MONKEY_SCOPED_TASK_IMPL__
