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
    template <typename Type>
    class AsyncProperty
    {
    public:
        AsyncProperty(const Bn3Tag& name, Type default_value) : _name(name), _value(default_value)
        {
            _on_property_changed_map = Bn3MapAllocator(Bn3Tag, std::function<bool(Type)>)(Bn3MapAllocator(Bn3Tag, std::function<bool(Type)>, Bn3Tag("map", name)));
        }

        int get()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                return _value;
            }
        }

        bool set(Type value)
        {
            onPropertyProcessed(value);
        }
        void setAsync(const Bn3Tag& scope_name, Type value)
        {
            ScopedTaskScope(scope_name).run(_name, &AsyncProperty<Type>::onPropertyProcessed, this, value);
        }

        void registerOnPropertyChanged(const Bn3Tag& scope_name, std::function<bool(Type value)> onPropertyChanged)
        {
            _on_property_changed_map[scope_name] = onPropertyChanged;
        }
        void clearOnPropertyChanged()
        {
            _on_property_changed.clear();
        }

        void registerOnPropertyNotified(const Bn3Tag& scope_name, std::function<void(Type, bool)> onPropertyNotified)
        {
            _notified_target_scope = scope_name;
            _on_property_notified = onPropertyNotified;
        }
        void clearOnPropertyNotified()
        {
            _notified_target_scope.clear();
            _on_property_notified = nullptr;
        }

    private:
        bool onPropertyChanged(Type value)
        {
            ScopedTaskResult<bool> results[32];
            size_t length = 0;

            for (auto& onpropertychanged : _on_property_changed_map)
            {
                auto& scope_name = onpropertychanged.first;
                auto& callback = onpropertychanged.second;

                results[length++] = ScopedTaskScope(scope_name).call(Bn3Tag("Changed_",_name), callback);
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
            if (_on_property_notified)
            {
                ScopedTaskScope(_notified_target_scope).run(Bn3Tag("Notified", name), _on_property_notified, value, success);
            }
        }
        void onPropertyProcessed(Type value)
        {
            bool ret = onPropertyChanged(value);
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _value = value;
            }
            onPropertyNotified(_value, ret);
        }


        Bn3Tag _name;
        int _value;
        std::mutex _mtx;

        Bn3MapAllocator(Bn3Tag, std::function<bool(int)>) _on_property_changed_map;
        
        std::function<void(int, bool)> _on_property_notified;
        Bn3Tag _notified_target_scope;
    };
}
#endif