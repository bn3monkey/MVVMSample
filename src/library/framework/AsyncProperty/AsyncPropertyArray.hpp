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
        OnPropertyArrayNotified(const ScopedTaskScope& scope, std::function<void(const Type(&)[], size_t, size_t, bool)> function) : scope(scope), function(function) {
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


}
#endif