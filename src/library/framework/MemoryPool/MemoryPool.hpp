#ifndef __BN3MONKEY_MEMORY_POOL__
#define __BN3MONKEY_MEMORY_POOL__

#include <memory>
#include "MemoryPoolImpl.hpp"

namespace Bn3Monkey
{
	class Bn3MemoryPool
	{
	public:

		template<typename... Sizes>
        static inline bool initialize(Sizes... sizes)
        {
			return _impl.initialize(std::forward<Sizes>(sizes)...);
        }

		static inline void release()
		{
			_impl.release();
		}

		template<class Type, class... Args>
        static inline Type* allocate_and_initialize(Args... args)
		{
			return _impl.allocate_and_initialize<Type>(std::forward<Args>(args)...);
		}

        template<class Type>
        static inline Type* allocate_and_initialize()
		{
			return _impl.allocate_and_initialize<Type>();
		}

        template<class Type>
        static inline Type* allocate(size_t size)
		{
			return _impl.allocate<Type>(size);
		}

		// In the case of a reference cast to the base class, you must call this method after casting it back to the derived class.
        template<class Type>
        static inline bool deallocate(Type* reference)
		{
			return _impl.deallocate(reference);
		}

		// In the case of a reference cast to the base class, you must call this method after casting it back to the derived class.
        template<class Type>
        static inline bool deallocate(Type* reference, size_t size)
		{
			return _impl.deallocate(reference, size);
		}
	
	private:
		static Bn3MemoryBlockPools<8> _impl;
	};

	template<class Type>
	class Bn3MemoryAllocator : public std::allocator<Type>
	{
	public:
		template<class Type_>
		struct rebind {
			typedef Bn3MemoryAllocator<Type_> other;
		};

		Bn3MemoryAllocator() noexcept {}
		
		template<class Type_>
		Bn3MemoryAllocator(const Bn3MemoryAllocator<Type_>& other) noexcept : std::allocator<Type>(other) {}

		Type* allocate(size_t n, const void* hint = 0)
		{
			return Bn3MemoryPool::allocate<Type>(n);
		}

		void deallocat(Type* p, size_t n)
		{
			Bn3MemoryPool::deallocate(p, n);
		}
	};
}

#endif