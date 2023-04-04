#ifndef __BN3MONKEY_MEMORY_POOL__
#define __BN3MONKEY_MEMORY_POOL__

#include <memory>

#include "MemoryPoolImpl.hpp"

#include "../Tag/Tag.hpp"

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
        static inline Type* construct(const Bn3Tag& tag , Args... args)
		{
			return _impl.construct<Type>(tag, std::forward<Args>(args)...);
		}


        template<class Type>
        static inline bool destroy(Type* ptr)
		{
			return _impl.destroy<Type>(ptr);
		}

		template<class Type>
		static inline Type* allocate(const Bn3Tag& tag, size_t size)
		{
			return _impl.allocate<Type>(tag, size);
		}


		// In the case of a reference cast to the base class, you must call this method after casting it back to the derived class.
        template<class Type>
        static inline bool deallocate(Type* reference, size_t size)
		{
			return _impl.deallocate<Type>(reference, size);
		}

		class Analyzer
		{
		public:
			std::string analyze() {
				return _impl.analysis();
			}
		};

	private:
		static Bn3MemoryBlockPools<8> _impl;
	};

	template<class Type, class... Args>
	inline std::shared_ptr<Type> makeSharedFromMemoryPool(const Bn3Tag& tag, Args... args) {
		auto* raw = Bn3MemoryPool::construct<Type>(tag, std::forward<Args>(args)...);
		if (!raw)
		{
			return nullptr;
		}
		auto ret = std::shared_ptr<Type>(raw, [&](Type* ptr) {
			Bn3MemoryPool::destroy(ptr);
			});
		return ret;
	}

	template<class Type, const char* TAG>
	class Bn3Allocator : public std::allocator<Type>
	{
	public:
		using value_type = Type;
		using pointer = Type*;
		using const_pointer = const Type*;
		using reference = Type&;
		using const_reference = const Type&;
		using size_type = size_t;
		using difference_type = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;

		Bn3Allocator() = default;
		
		template <typename U>
		Bn3Allocator(const Bn3Allocator<U, TAG>& other) noexcept {}

		~Bn3Allocator() noexcept {}

		template <class U>
		struct rebind { 
			using other = Bn3Allocator<U, TAG> ;
		};

		pointer allocate(size_type n, const void* hint = 0)
		{
			return Bn3MemoryPool::allocate<value_type>(Bn3Tag(TAG), n);
		}
		void deallocate(pointer ptr, size_type n) noexcept {
			Bn3MemoryPool::deallocate<value_type>(ptr, n);
		}
	};
}

#endif