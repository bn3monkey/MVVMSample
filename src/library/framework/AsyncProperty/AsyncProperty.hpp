#ifndef __BN3MONKEY_ASYNC_PROPERTY__
#define __BN3MONKEY_ASYNC_PROPERTY__

#include "../Tag/Tag.hpp"
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


#include "../ScopedTask/ScopedTask.hpp"
#include <functional>

namespace Bn3Monkey
{
    template<typename Type>
    class OnPropertyChanged {
    public:
        OnPropertyChanged(const ScopedTaskScope& scope, std::function<bool(Type)> function) : scope(scope), function(function) {}
        ScopedTaskResult<bool> operator()(const Bn3Tag& name, Type value) {
            return scope.call(name, function, value);
        }
    private:
        ScopedTaskScope scope;
        std::function<bool(Type)> function;
    };

    template<typename Type>
    class OnPropertyNotified
    {
    public:
        OnPropertyNotified() {
            is_intialized = false;
        }
        OnPropertyNotified(const ScopedTaskScope& scope, std::function<void(Type, bool)> function) : scope(scope), function(function) {
            is_intialized = true;
        }
        void operator()(const Bn3Tag& name, Type value, bool success) {
            if (!is_initalized)
                return;
            scope.run(name, function, value, success);
        }
        void clear() {
            is_initialized = false;
        }
    private:
        bool is_initalized;
        ScopedTaskScope scope;
        std::function<void(Type, bool)> function;
    };

    template <typename Type>
    class AsyncProperty
    {
        

    public:
        AsyncProperty(const Bn3Tag& name, Type default_value) : _name(name), _value(default_value)
        {
            _on_property_changed = Bn3Vector(OnPropertyChanged<Type>)(Bn3VectorAllocator(OnPropertyChanged<Type>, Bn3Tag("changed_", name)));
        }

        int get()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                return _value;
            }
        }

        bool set(const ScopedTaskScope& scope, Type value)
        {
            auto result = scope.call(_name, &AsyncProperty<Type>::onPropertyProcessed, this, value);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }
        void setAsync(const ScopedTaskScope& scope, Type value)
        {
            scope.run(_name, &AsyncProperty<Type>::onPropertyProcessed, this, value);
        }

        void registerOnPropertyChanged(const ScopedTaskScope& scope, std::function<bool(Type value)> onPropertyChanged)
        {
            _on_property_changed.emplace_back(scope, onPropertyChanged);
        }
        void clearOnPropertyChanged()
        {
            _on_property_changed.clear();
        }

        void registerOnPropertyNotified(const ScopedTaskScope& scope, std::function<void(Type, bool)> onPropertyNotified)
        {
            _on_property_notified = OnPropertyNotified<Type>(scope, onPropertyNotified);
        }
        void clearOnPropertyNotified()
        {
            _on_property_notified.clear();
        }

    private:
        bool onPropertyChanged(Type value)
        {
            ScopedTaskResult<bool> results[32];
            size_t length = 0;

            for (auto& onpropertychanged : _on_property_changed)
            {
                results[length++] = onpropertychanged(Bn3Tag("Changed_",_name), value);
            }
            
            for (size_t i = 0; i < length; i++)
            {
                bool* ret = results.wait();
                if (!ret)
                    return false;
                if (!(*ret))
                    return false;
            }

            return true;
        }
        void onPropertyNotified(Type value, bool success)
        {
            _on_property_notified(Bn3Tag("Notified_", name), value, success);
        }
        bool onPropertyProcessed(Type value)
        {
            bool ret = onPropertyChanged(value);
            if (ret)
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _value = value;
            }
            onPropertyNotified(_value, ret);
            return ret;
        }


        Bn3Tag _name;
        Type _value;
        std::mutex _mtx;
       
        Bn3Vector(OnPropertyChanged<Type>) _on_property_changed;
        OnPropertyNotified<Type> _on_property_notified;
    };
}
#endif