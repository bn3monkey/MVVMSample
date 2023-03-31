#ifndef __BN3MONKEY_MEMORY_POOL__
#define __BN3MONKEY_MEMORY_POOL__

#include <vector>
#include <cstdint>
#include <initializer_list>
#include <mutex>

namespace Bn3Monkey
{
	class Bn3MemoryPool
	{
	public:
		
		// constexpr static size_t HEADER_SIZE = 32;
		constexpr static size_t BLOCK_SIZE_POOL[] = { 32, 64, 128, 256, 512, 1024, 4096, 8192 };
		constexpr static size_t BLOCK_SIZE_POOL_SIZE = sizeof(BLOCK_SIZE_POOL)/sizeof(size_t);
		
		class Header
		{
			char* freed_ptr;
		};
		constexpr static size_t BLOCK_HEADER_SIZE = sizeof(Header);

		constexpr static size_t BLOCK_CONTENT_SIZE_POOL[] = 
		{ BLOCK_SIZE_POOL[0] - BLOCK_HEADER_SIZE,
		  BLOCK_SIZE_POOL[1] - BLOCK_HEADER_SIZE,
		BLOCK_SIZE_POOL[2] - BLOCK_HEADER_SIZE,
		BLOCK_SIZE_POOL[3] - BLOCK_HEADER_SIZE,
		BLOCK_SIZE_POOL[4] - BLOCK_HEADER_SIZE,
		BLOCK_SIZE_POOL[5] - BLOCK_HEADER_SIZE,
		BLOCK_SIZE_POOL[6] - BLOCK_HEADER_SIZE };

		static void initialize(std::initializer_list<size_t> pool_sizes)
		{
			constexpr size_t min_size = BLOCK_SIZE_POOL_SIZE < pool_sizes.size() ? BLOCK_SIZE_POOL_SIZE : pool_sizes.size();
			for (size_t i = 0; i < min_size; i++)
			{
				size_t pool_size = pool_sizes[i];
				size_t block_size = BLOCK_SIZE_POOL[i];

				auto& _pool = _pools[i];
				_pool.resize(pool_size * block_size);
				
				auto& ptr = latest_freed_ptrs[i];

				if (pool_size > 0)
				{
					auto& latest_freed_ptr = latest_freed_ptrs[i];
					auto* block = getBlock(pool, block_size, i);
					latest_freed_ptr = block;


					for (size_t i = 1; i < pool_size - 1; i++)
					{
						auto* block = getBlock(pool, block_size, i);
						Header& header = getHeader(block, block_size);
						header.freed_ptr = getBlock(pool, block_size, i+1)
					}
					if (pool_size > 1)
					{
						auto* block = getBlock(pool, pool_size - 1, i);
						Header& header = getHeader(block, block_size);
						header.freed_ptr = nullptr;
					}
				}
			}
		}
		static void release()
		{
			for (auto& pool : _pools)
			{
				pool.clear();
			}
		}

		void* allocate(size_t object_size, size_t length)
		{
			analysis.up();
			size_t pool_number = getPoolNumber(object_size * length);
			if (pool_number >= BLOCK_SIZE_POOL_SIZE)
				return new char[object_size * length];
			else
			{
				
			}
		}
		
	private:
		inline constexpr size_t getPoolNum(size_t object_size)
		{
			for (size_t i = 0; i < BLOCK_SIZE_POOL_SIZE; i++)
			{
				if (object_size <= BLOCK_CONTENT_SIZE_POOL[i])
					return i;
			}
			return BLOCK_SIZE_POOL_SIZE;
		}
		inline constexpr char* getBlock(std::vector<char>& pool, size_t block_size, size_t offset)
		{
			auto* data = pool.data();
			return reinterpret_cast<char *>(data + block_size * offset);
		}
		inline constexpr Header& getHeader(char* block, size_t block_size)
		{
			auto* header_ptr = block + block_size;
			return *reinterpret_cast<Header*>(header_ptr);
		}


		static std::vector<char> _pools[BLOCK_SIZE_POOL_SIZE];
		static char* latest_freed_ptrs[BLOCK_SIZE_POOL_SIZE]{ nullptr };
		static std::mutex _mtx;

		class Analysis
		{
			size_t max_allocated{ 0 };
			size_t current_allocated {0};

			size_t max_deallocated{ 0 };
			size_t current_deallocated{ 0 };

			void up() {
				current_allocated += 1;
				if (max_allocated < current_allocated)
					max_allocated = current_allocated;
			}
			void down() {
				max_deallocated += 1;
				if (max_deallocated < current_deallocated)
					max_deallocated = current_deallocated;
			}
		};
		static Analysis analysis;
	};
}

#endif