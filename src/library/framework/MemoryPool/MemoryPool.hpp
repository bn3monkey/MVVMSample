#ifndef __BN3MONKEY_MEMORY_POOL__
#define __BN3MONKEY_MEMORY_POOL__

#include <memory>
#include <list>
#include <vector>
#include <deque>
#include <queue>
#include <unordered_map>
#include <string>

#include "MemoryPoolImpl.hpp"

#include "../Tag/Tag.hpp"

namespace Bn3Monkey
{
	class Bn3MemoryPool
	{
	public:

        static inline bool initialize(std::initializer_list<size_t> sizes)
        {
			return _impl.initialize(sizes);
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
			std::string analyzeAll() {
				return _impl.analyzeAll();
			}
			
			std::string analyzePool(size_t i) {
				return _impl.analyzePool(i);
			}
		};

	private:
		static Bn3MemoryBlockPools<BLOCK_SIZE_POOL_LENGTH> _impl;
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

	template<class Type>
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
		Bn3Allocator(const Bn3Tag& tag)
		{
			_tag = tag;
		}

		template <typename U>
		Bn3Allocator(const Bn3Allocator<U>& other) noexcept : _tag(other._tag) {}

		~Bn3Allocator() noexcept {}

		template <class U>
		struct rebind { 
			using other = Bn3Allocator<U> ;
		};

		pointer allocate(size_type n, const void* hint = 0)
		{
			return Bn3MemoryPool::allocate<value_type>(_tag, n);
		}
		void deallocate(pointer ptr, size_type n) noexcept {
			Bn3MemoryPool::deallocate<value_type>(ptr, n);
		}

		template<class... Args>
		void construct(pointer ptr, Args&&... values)
		{
			new (ptr) value_type(std::forward<Args>(values)...);
		}
		void destroy(pointer ptr)
		{
			ptr->~value_type();
		}

		Bn3Tag _tag;
	};

	class Bn3Container
	{
	public:
		using string = std::basic_string<char, std::char_traits<char>, Bn3Allocator<char>>;

		template<class Type>
		using list = std::list<Type, Bn3Allocator<Type>>;

		template<class Type>
		using deque = std::deque<Type, Bn3Allocator<Type>>;

		template<class Type>
		using vector = std::vector<Type, Bn3Allocator<Type>>;

		template<class Type>
		using queue = std::queue<Type, std::deque<Type, Bn3Allocator<Type>>>;

		template<class Key, class Value>
		using map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, Bn3Allocator<std::pair<const Key, Value>>>;
	};

}

#endif