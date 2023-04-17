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
    
    template<typename Type, size_t MAX_ARRAY_SIZE>
    class OnPropertyArrayNotified
    {
    public:
        OnPropertyArrayNotified(const ScopedTaskScope& scope, std::function<bool(const Type*, size_t, size_t)> function) : scope(scope), function(function) {
        }
        ScopedTaskResult<bool> operator()(const Bn3Tag& name, const Type* values, size_t offset, size_t length) {
            return scope.call(name, function, values, offset, length);
        }
    private:
        ScopedTaskScope scope;
        std::function<bool(const Type*, size_t, size_t)> function;
    };

    template<typename Type, size_t MAX_ARRAY_SIZE>
    class OnPropertyArrayUpdated
    {
    public:
        OnPropertyArrayUpdated(const ScopedTaskScope& scope, std::function<void(const Type*, size_t, size_t, bool)> function) : scope(scope), function(function) {
        }
        void operator()(const Bn3Tag& name, const Type* values, size_t offset, size_t length, bool success) {
            scope.run(name, function, values, offset, length, success);
        }
    private:
        ScopedTaskScope scope;
        std::function<void(const Type*, size_t, size_t, bool)> function;
    };

    
    template <typename Type, size_t MAX_ARRAY_SIZE>
    class AsyncPropertyArray : public AsyncPropertyNode
    {
    public:
        static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_same_v<Type, std::string>);

        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, const Type (&values)[MAX_ARRAY_SIZE], size_t length) : _name(name), _scope(scope), _length(length)
        {
            assert(length <= MAX_ARRAY_SIZE);

            _prev_values.copyFrom(values, 0, length);
            _values.copyFrom(values, 0, length);

        }

        template<size_t array_size>
        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, std::initializer_list<Type> values) : _name(name), _scope(scope)
        {
            static_assert(array_size >= MAX_ARRAY_SIZE);
            assert(length <= MAX_ARRAY_SIZE);

            _length = values.size();

            _prev_values.copyFrom(values);
            _values.copyFrom(values);

        }


        virtual bool isValid(const Type* values, size_t offset, size_t length)
        {
            if (offset >= _length)
                return false;
            if (offset + length >= _length)
                return false;
            return true;
        }        


        bool get(Type* values, size_t offset, size_t length)
        {
            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyObtained, this, values, offset, length);
            auto ret = result.wait();
            if (!ret)
                return false;
            return ret;
        }

        Type get(size_t idx)
        {
            Type value = _values.data[idx];
            bool ret = get(&value, idx, 1);
            if (!ret)
                return value;
            return value;
        }

        bool set(const Type* values, size_t offset, size_t length)
        {
            if (!isValid(values, offset, length))
                return false;

            Array temp{_values};
            temp.copyFrom(values, offset, length);

            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyCommitted, this, temp);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        inline bool set(const Type& value, size_t idx)
        {
            return set(&value, idx, 1);
        }

        bool notify(size_t offset, size_t length)
        {
            auto result = _scope.call(_name, AsyncPropertyArray::onPropertyNotified, this, offset, length);
            auto ret = result.wait();
            if (!ret)
                return false;
            if (!(*ret))
                return false;
            return true;
        }

        inline bool notify(size_t idx)
        {
            return notify(idx, 1);
        }

        void update(size_t offset, size_t length, bool success)
        {
            _scope.run(_name, AsyncPropertyArray::onPropertyUpdated, this, offset, length, success);
        }

        inline void update(size_t idx, bool success)
        {
            update(idx, 1, success);
        }

        void setAsync(const Type* values, size_t offset, size_t length)
        {
            if (!isValid(values, offset, length))
                return;

            Array temp{ _values };
            temp.copyFrom(values, offset, length);

            _scope.run(_name, AsyncPropertyArray::onPropertyProcessed, this, temp, offset, length);
        }

        inline void setAsync(const Type& value, size_t idx)
        {
            return setAsync(&value, idx, 1);
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

        struct Array
        {
            Type data[MAX_ARRAY_SIZE];
            
            Array()
            {
            }
            Array(const Array& other)
            {
                std::copy(other.data, other.data + MAX_ARRAY_SIZE, data);
            }
            Array& operator=(const Array& other)
            {
                std::copy(other.data, other.data + MAX_ARRAY_SIZE, data);
                return *this;
            }
            void copyFrom(std::initializer_list<Type> values)
            {
                std::copy(values.begin(), values.end(), data);
            }
            void copyFrom(const Type* values, size_t offset, size_t length)
            {
                std::copy(values, values + length, data + offset);
            }
            void copyTo(Type* values, size_t offset, size_t length)
            {
                std::copy(data + offset, data + offset + length, values);
            }

        };

        static bool onPropertyObtained(AsyncPropertyArray* self, Type* values, size_t offset, size_t length)
        {
            self->_values.copyTo(values, offset, length);
            return true;
        }
        static bool onPropertyCommitted(AsyncPropertyArray* self, Array values)
        {
            self->_prev_values = self->_values;
            self->_values = values;
            return true;
        }
        static bool onPropertyNotified(AsyncPropertyArray* self, size_t offset, size_t length)
        {
            ScopedTaskResult<bool> results[8];
            size_t callback_length = 0;

            for (auto& on_property_notified : self->_on_property_notifieds)
            {
                auto result = on_property_notified(Bn3Tag("Notified_", self->_name), self->_values.data + offset, offset, length);
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
        static void onPropertyUpdated(AsyncPropertyArray* self, size_t offset, size_t length, bool success)
        {
            for (auto& on_property_updated : self->_on_property_updateds)
            {
                on_property_updated(Bn3Tag("Updated_", self->_name), self->_values.data + offset, offset, length, success);
            }
        }
        static bool onPropertyProcessed(AsyncPropertyArray* self, Array values, size_t offset, size_t length)
        {
            onPropertyCommitted(self, values);
            bool ret = onPropertyNotified(self, offset, length);
            onPropertyUpdated(self, offset, length, ret);
            return ret;
        }

        Bn3Tag _name;
        ScopedTaskScope _scope;

        Bn3StaticVector<OnPropertyArrayNotified<Type, MAX_ARRAY_SIZE>, 16> _on_property_notifieds;
        Bn3StaticVector<OnPropertyArrayUpdated<Type, MAX_ARRAY_SIZE>, 16> _on_property_updateds;

        size_t _length;

        Array _prev_values;
        Array _values;
    };
}

#endif