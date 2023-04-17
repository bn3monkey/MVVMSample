#ifndef __BN3MONKEY_ASYNC_PROPERTY__
#define __BN3MONKEY_ASYNC_PROPERTY__

#include "../Tag/Tag.hpp"
#include "../StaticVector/StaticVector.hpp"

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
        static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_same_v<Type, std::string>);


        AsyncProperty(const Bn3Tag& name, const ScopedTaskScope& scope, const Type& default_value) : _name(name), _scope(scope), _value(default_value), _prev_value(default_value)
        {
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
       
        Bn3StaticVector<OnPropertyNotified<Type>, 16> _on_property_notifieds;
        Bn3StaticVector<OnPropertyUpdated<Type>, 16> _on_property_updateds;

        Type _prev_value;
        Type _value;
    };

}
#endif