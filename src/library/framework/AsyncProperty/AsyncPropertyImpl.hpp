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
#include "AsyncPropertyNode.hpp"


#include <functional>
#include <initializer_list>
#include <type_traits>

namespace Bn3Monkey
{
    template<typename Type>
    class OnPropertyNotified {
    public:        
        OnPropertyNotified(const ScopedTaskScope& scope, std::function<bool(const Type&)> function) : scope(scope), function(function) {
        }
        ScopedTaskResult<bool> operator()(const Bn3Tag& name, const Type& value) {
            return scope.call(name, function, value);
        }

    private:
        ScopedTaskScope scope;
        std::function<bool(const Type&)> function;
    };

    template<typename Type>
    class OnPropertyUpdated
    {
    public:
        OnPropertyUpdated(const ScopedTaskScope& scope, std::function<void(const Type&, bool)> function) : scope(scope), function(function) {
        }
        void operator()(const Bn3Tag& name, const Type& value, bool success) {
            scope.run(name, function, value, success);
        }

    private:
        ScopedTaskScope scope;
        std::function<void(const Type&, bool)> function;
    };

    
        

    template <typename Type>
    class AsyncProperty : public AsyncPropertyNode
    {
    public:
        static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_same_v<Type, Bn3String()>);


        AsyncProperty(const Bn3Tag& name, const ScopedTaskScope& scope, const Type& default_value) : _name(name), _scope(scope), _value(default_value), _prev_value(default_value)
        {
            _on_property_notifieds = Bn3Vector(OnPropertyNotified<Type>)(Bn3VectorAllocator(OnPropertyNotified<Type>, Bn3Tag("Notifier_", name)));
            _on_property_notifieds.reserve(32);
            _on_property_updateds = Bn3Vector(OnPropertyUpdated<Type>)(Bn3VectorAllocator(OnPropertyUpdated<Type>, Bn3Tag("Updater_", name)));
            _on_property_updateds.reserve(32);
        }

        virtual bool isValid(const Type& value)
        {
            return true;
        }

        Type get()
        {
            auto result = _scope.call(_name, AsyncProperty<Type>::onPropertyObtained, this);
            auto ret = result.wait();
            if (!ret)
                return _value;
            if (!(*ret))
                return _value;
            return *ret;
        }

        bool set(const Type& value)
        {
            if (!isValid(value))
                return false;

            auto result = _scope.call(_name, AsyncProperty<Type>::onPropertyCommitted, this, value);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        bool notify()
        {
            auto result = _scope.call(_name, AsyncProperty<Type>::onPropertyNotified, this);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        void update(bool success)
        {
            _scope.run(_name, AsyncProperty<Type>::onPropertyUpdated, this, success);
        }

        void setAsync(const Type& value)
        {
            if (!isValid(value))
                return;

            _scope.run(_name, AsyncProperty<Type>::onPropertyProcessed, this, value);
        }
                        

        void registerOnPropertyNotified(const ScopedTaskScope& scope, std::function<bool(const Type&)> onPropertyNotified)
        {
            _on_property_notifieds.emplace_back(scope, onPropertyNotified);
        }

        void clearOnPropertyNotified()
        {
            _on_property_notifieds.clear();
        }

        void registerOnPropertyUpdated(const ScopedTaskScope& scope, std::function<void(const Type&, bool)> onPropertyUpdated)
        {
            _on_property_updateds.emplace_back(scope, onPropertyUpdated);
        }

        void clearOnPropertyUpdated()
        {
            _on_property_updateds.clear();
        }

    private:
        static Type onPropertyObtained(AsyncProperty<Type>* self)
        {
            return self->_value;
        }
        static bool onPropertyCommitted(AsyncProperty<Type>* self, const Type& value)
        {
            self->_prev_value = self->_value;
            self->_value = value;
            return true;
        }
        static bool onPropertyNotified(AsyncProperty<Type>* self)
        {
            ScopedTaskResult<bool> results[8];
            
            size_t length = 0;

            for (auto& on_property_notified : self->_on_property_notifieds)
            {
                auto result = on_property_notified(Bn3Tag("Notified_", self->_name), self->_value);
                results[length++] = std::move(result);
            }
            
            bool success = true;
            for (size_t i = 0; i < length; i++)
            {
                bool* ret = results[i].wait();
                if (!ret)
                {
                    success = false;
                    break;
                }
                if (!(*ret))
                {
                    success = false;
                    break;
                }
            }

            if (success)
            {
                // confirmed
            }
            else
            {
                self->_value = self->_prev_value;
            }
            return success;
        }
        static void onPropertyUpdated(AsyncProperty<Type>* self, bool success)
        {
            for (auto& on_property_updated : self->_on_property_updateds)
            {
                on_property_updated(Bn3Tag("Updated_", self->_name), self->_value, success);
            }
        }
        static bool onPropertyProcessed(AsyncProperty<Type>* self, const Type& value)
        {
            onPropertyCommitted(self, value);
            bool ret = onPropertyNotified(self);
            onPropertyUpdated(self, ret);
            return ret;
        }


        Bn3Tag _name;
        ScopedTaskScope _scope;
       
        Bn3Vector(OnPropertyNotified<Type>) _on_property_notifieds;
        Bn3Vector(OnPropertyUpdated<Type>) _on_property_updateds;

        Type _prev_value;
        Type _value;
    };

}
#endif