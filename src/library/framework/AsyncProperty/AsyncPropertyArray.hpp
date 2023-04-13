#ifndef __BN3MONKEY_ASYNC_PROPERTY_ARRAY__
#define __BN3MONKEY_ASYNC_PROPERTY_ARRAY__

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
    class OnPropertyArrayChanged
    {
    public:
        OnPropertyArrayChanged() {
            is_initialized = false;
        }
        OnPropertyArrayChanged(const ScopedTaskScope& scope, std::function<bool(const Type(&)[], size_t, size_t)> function) : scope(scope), function(function) {
            is_initialized = true;
        }
        ScopedTaskResult<bool> operator()(const Bn3Tag& name, const Type(&values)[], size_t offset, size_t index) {
            if (!is_initialized)
                return;
            return scope.call(name, function, values, offset, index);
        }
    private:
        bool is_initialized;
        std::function<void(const Type(&)[], size_t, size_t)> function
    };

    template<typename Type>
    class OnPropertyArrayNotified
    {
    public:
        OnPropertyArrayNotified() {
            is_initialized = false;
        }
        OnPropertyArrayNotified(const ScopedTaskScope& scope, std::function<void(const Type&, size_t, size_t, bool)> function) : scope(scope), function(function) {
            is_initialized = true;
        }
        void operator()(const Bn3Tag& name, const Type(&values)[], size_t offset, size_t index) {
            if (!is_initialized)
                return;
            scope.run(name, function, values, offset, index);
        }
        void clear() {
            is_initialized = false;
        }
    private:
        bool is_initialized;
        std::function<void(const Type(&)[], size_t, size_t)> function
    };

    template <typename Type, size_t MAX_ARRAY_SIZE>
    class AsyncPropertyArray : public AsyncPropertyNode
    {
    public:
        static_assert(std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_same_v<Type, Bn3String()>);

        template<size_t array_size>
        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, const Type (&values)[array_size], size_t length) : _name(name), _scope(scope), _length(length)
        {
            static_assert(array_size >= MAX_ARRAY_SIZE);
            assert(length <= MAX_ARRAY_SIZE);

            std::copy(values, values + length, _values);
        }

        template<size_t array_size>
        AsyncPropertyArray(const Bn3Tag& name, const ScopedTaskScope& scope, std::initializer_list<Type> values) : _name(name), _scope(scope)
        {
            static_assert(array_size >= MAX_ARRAY_SIZE);
            assert(length <= MAX_ARRAY_SIZE);

            _length = values.size();
            std::copy(values.begin(), values.end(), _values);
        }

        template<size_t array_size>
        void initialize(const Type(&values)[array_size], size_t length)
        {
            static_assert(array_size >= MAX_ARRAY_SIZE);
            assert(length <= MAX_ARRAY_SIZE);

            _length = length;
            std::copy(values, values + length, _values);
        }

        void initialize(std::initializer_list<Type> values)
        {
            _length = length;
            std::copy(values.begin(), values.end(), _values);
        }

        virtual bool isValid(Type* values, size_t offset, size_t length)
        {
            if (offset >= _length)
                return false;
            if (offset + length >= _length)
                return false;
            return true;
        }

        void signal()
        {
            _scope.run(_name, &AsyncPropertyArray<Type>::onPropertyProcessed, this, _value);
        }


        bool get(Type* values, size_t offset, size_t length)
        {
            auto result = _scope.call(_name, &AsyncPropertyArray<Type>::onPropertyGetted, this, values, offset, length);
            auto ret = result.wait();
            if (!ret)
                return false;
            return ret;
        }

        Type get(size_t idx)
        {
            auto value = _value;
            bool ret = get(&value, idx, 1);
            if (!ret)
                return value;
            return value;
        }



        bool set(Type* values, size_t offset, size_t length)
        {
            if (!isValid(values, offset, length))
                return false;

            Setter setter{ values, offset, length };
            auto result = _scope.call(_name, &AsyncPropertyArray<Type>::onPropertyProcessed, this, setter);
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

        bool setAsync(Type* values, size_t offset, size_t length)
        {
            if (!isValid(values, offset, length))
                return false;

            Setter setter{ values, offset, length };
            _scope.run(_name, &AsnycPropertyArray<Type>::onPropertyProcessed, this, setter);
        }

        inline void setAsync(const Type& value, size_t idx)
        {
            return setAsync(&value, idx, 1);
        }

        void registerOnPropertyChanged(const ScopedTaskScope& scope, std::function<void(const Type(&)[], size_t, size_t)> onPropertyChanged)
        {
            _on_property_changed.emplace_back(scope, onPropertyChanged);
        }

        void clearOnPropertyChanged()
        {
            _on_property_changed.clear();
        }

        void registerOnPropertyNotified(const ScopedTaskScope& scope, std::function<void(const Type(&)[], size_t, size_t)> onPropertyNotified)
        {
            _on_property_notified = OnPropertyArrayNotified<Type>(scope, onPropertyNotified);
        }

        void clearOnPropertyNotified()
        {
            _on_property_notified.clear();
        }

    private:
        struct Setter{
            Type _values[MAX_ARRAY_SIZE];
            size_t _offset;
            size_t _length;

            Setter(Type* values, size_t offset, size_t length)
            {
                _offset = offset;
                _length = length;
                std::copy(values, values + length, _values);
            }
        }

        Bn3Tag _name;
        ScopedTaskScope _scope;

        Bn3Vector(OnPropertyArrayChanged<Type>) _on_property_changed;
        OnPropertyArrayNotified<Type> _on_property_notified;

        size_t _length;
        Type _values[MAX_ARRAY_SIZE];
    };
}

#endif