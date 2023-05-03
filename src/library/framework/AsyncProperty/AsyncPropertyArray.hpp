#ifndef __BN3MONKEY_ASYNC_PROPERTY_ARRAY__
#define __BN3MONKEY_ASYNC_PROPERTY_ARRAY__

#include "../Tag/Tag.hpp"
#include "../MemoryPool/MemoryPool.hpp"
#include "../StaticVector/StaticVector.hpp"
#include "../ScopedTask/ScopedTask.hpp"
#include "AsyncPropertyNode.hpp"

#include <functional>
#include <initializer_list>
#include <type_traits>

namespace Bn3Monkey
{
    
    template<typename Type>
    class OnPropertyArrayNotified
    {
    public:
        OnPropertyArrayNotified(const ScopedTaskScope& scope, std::function<bool(const Type*, size_t, size_t)> function) : scope(scope), function(function) {
        }
        ScopedTaskResult<bool> operator()(const Bn3Tag& name, const Type* values, size_t start, size_t end) {
            return scope.call(name, function, values, start, end);
        }
    private:
        ScopedTaskScope scope;
        std::function<bool(const Type*, size_t, size_t)> function;
    };

    template<typename Type>
    class OnPropertyArrayUpdated
    {
    public:
        OnPropertyArrayUpdated(const ScopedTaskScope& scope, std::function<void(const Type*, size_t, size_t, bool)> function) : scope(scope), function(function) {
        }
        void operator()(const Bn3Tag& name, const Type* values, size_t start, size_t end, bool success) {
            scope.run(name, function, values, start, end, success);
        }
    private:
        ScopedTaskScope scope;
        std::function<void(const Type*, size_t, size_t, bool)> function;
    };

    
    template <typename Type, size_t MAX_ARRAY_SIZE>
    class AsyncPropertyArray : public AsyncPropertyNode
    {
    public:
        static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_same_v<Type, Bn3StaticString>);

        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, const Type* values, size_t length) : _name(name), _scope(scope), _length(length)
        {
            assert(length <= MAX_ARRAY_SIZE);

            _prev_values = Bn3StaticVector<Type, MAX_ARRAY_SIZE>(values, length);
            _values = _prev_values;

        }

        template<size_t array_size>
        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, std::initializer_list<Type> values) : _name(name), _scope(scope), _length(values.size())
        {
            static_assert(array_size >= MAX_ARRAY_SIZE);
            assert(_length <= MAX_ARRAY_SIZE);

            _prev_values = Bn3StaticVector<Type, MAX_ARRAY_SIZE>(values);
            _values = _prev_values;

        }


        virtual bool isValid(const Type* values, size_t start, size_t end)
        {
            if (end <= start)
                return false;
            if (end > _length)
                return false;
            return true;
        }        

        const size_t length() noexcept { return _length; }

        bool get(Type* values, size_t start, size_t end)
        {
            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyObtained, this, values, start, end);
            auto ret = result.wait();
            if (!ret)
                return false;
            return ret;
        }

        inline bool set(const Type* values, size_t start, size_t end)
        {
            if (!isValid(values, start, end))
                return false;


            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyCommitted, this, values, start, end);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        bool notify(size_t start, size_t end)
        {
            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyNotified, this, start, end);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        void update(size_t start, size_t end, bool success)
        {
            _scope.run(_name, AsyncPropertyArray::onPropertyUpdated, this, start, end, success);
        }


        void setAsync(const Type* values, size_t start, size_t end)
        {
            if (!isValid(values, start, end))
                return;

            _scope.run(_name, AsyncPropertyArray::onPropertyProcessed, this, values, start, end);
        }

        void registerOnPropertyNotified(const ScopedTaskScope& scope, std::function<bool(const Type*, size_t, size_t)> onPropertyNotified)
        {
            _on_property_notifieds.emplace_back(scope, onPropertyNotified);
        }

        void clearOnPropertyNotified()
        {
            _on_property_notifieds.clear();
        }

        void registerOnPropertyUpdated(const ScopedTaskScope& scope, std::function<void(const Type*, size_t, size_t, bool)> onPropertyUpdated)
        {
            _on_property_updateds.emplace_back(scope, onPropertyUpdated);
        }

        void clearOnPropertyUpdated()
        {
            _on_property_updateds.clear();
        }

    private:

        

        static bool onPropertyObtained(AsyncPropertyArray* self, Type* values, size_t start, size_t end)
        {
            self->_values.copyTo(values, start, end);
            return true;
        }
        static bool onPropertyCommitted(AsyncPropertyArray* self, const Type* values, size_t start, size_t end)
        {
            self->_prev_values = self->_values;
            self->_values.copyFrom(values, start, end);
            return true;
        }
        static bool onPropertyNotified(AsyncPropertyArray* self, size_t start, size_t end)
        {
            ScopedTaskResult<bool> results[8];
            size_t callback_length = 0;

            for (auto& on_property_notified : self->_on_property_notifieds)
            {
                auto result = on_property_notified(Bn3Tag("Notified_", self->_name), self->_values.begin() + start, start, end);
                results[callback_length++] = std::move(result);
            }

            bool success = true;
            for (size_t i = 0; i < callback_length; i++)
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
                self->_prev_values = self->_values;
            }
            return success;
        }
        static void onPropertyUpdated(AsyncPropertyArray* self, size_t start, size_t end, bool success)
        {
            for (auto& on_property_updated : self->_on_property_updateds)
            {
                on_property_updated(Bn3Tag("Updated_", self->_name), self->_values.begin() + start, start, end, success);
            }
        }
        static bool onPropertyProcessed(AsyncPropertyArray* self, const Type* values, size_t start, size_t end)
        {
            onPropertyCommitted(self, values, start, end);
            bool ret = onPropertyNotified(self, start, end);
            onPropertyUpdated(self, start, end, ret);
            return ret;
        }

        Bn3Tag _name;
        ScopedTaskScope _scope;

        Bn3StaticVector<OnPropertyArrayNotified<Type>, 4> _on_property_notifieds;
        Bn3StaticVector<OnPropertyArrayUpdated<Type>, 4> _on_property_updateds;

        size_t _length;

        Bn3StaticVector<Type, MAX_ARRAY_SIZE> _prev_values;
        Bn3StaticVector<Type, MAX_ARRAY_SIZE> _values;
    };
}

#endif