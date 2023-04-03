#include <MemoryPool/MemoryPool.hpp>

#include "test_helper.hpp"
#include "../test_helper.hpp"


constexpr size_t block_sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, 4098, 8192 };


template<size_t idx>
void inner_test_alloc(BlockBase* (&ptr)[9][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[idx];

	ptr[idx][0] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[idx][1] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[idx][2] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[idx][3] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();

	ptr[idx][4] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	if (ptr[idx][4] == nullptr)
	{
		say("Good! (Allocate Full check)");
	}
	
	inner_test_alloc<idx - 1>(ptr);
}
template<>
void inner_test_alloc<0>(BlockBase* (&ptr)[9][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[0];

	ptr[0][0] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[0][1] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[0][2] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	ptr[0][3] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();

	ptr[0][4] = Bn3MemoryPool::allocate_and_initialize < Block<block_size> >();
	if (ptr[0][4] == nullptr)
	{
		say("Good! (Allocate Full check)");
	}
}

template<size_t idx>
void inner_test_dealloc(BlockBase* (&ptr)[9][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[idx];

	Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][0]);
	ptr[idx][4] = Bn3MemoryPool::allocate_and_initialize < Block<block_size >>();

	Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][1]);
	Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][2]);
	Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][3]);
	Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][4]);

	if (!Bn3MemoryPool::deallocate((Block<block_size> *)ptr[idx][4]))
	{
		say("Good! (Double Free check)");
	}

	inner_test_dealloc<idx - 1>(ptr);
}
template<>
void inner_test_dealloc<0>(BlockBase* (&ptr)[9][5])
{
	using namespace Bn3Monkey;
	
	constexpr size_t block_size = block_sizes[0];

	Bn3MemoryPool::deallocate(ptr[0][0]);
	ptr[0][4] = Bn3MemoryPool::allocate_and_initialize < Block<block_size >>();

	Bn3MemoryPool::deallocate(ptr[0][1]);
	Bn3MemoryPool::deallocate(ptr[0][2]);
	Bn3MemoryPool::deallocate(ptr[0][3]);
	Bn3MemoryPool::deallocate(ptr[0][4]);

	if (!Bn3MemoryPool::deallocate(ptr[0][4]))
	{
		say("Good! (Double Free check)");
	}
}

void test_alloc(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize(4, 4, 4, 4, 4, 4, 4, 4);
	BlockBase* ptr[9][5];

	inner_test_alloc<8>(ptr);
	inner_test_dealloc<8>(ptr);

	{
		auto* ptr = Bn3MemoryPool::allocate_and_initialize < Block<10000 >> ();
		auto* ptr2 = Bn3MemoryPool::allocate_and_initialize < Block<10000 >> ();
		auto* ptr3 = Bn3MemoryPool::allocate_and_initialize < Block<10000 >> ();
		auto* ptr4 = Bn3MemoryPool::allocate_and_initialize < Block<10000 >> ();
		auto* ptr5 = Bn3MemoryPool::allocate_and_initialize < Block<10000 >> ();

		Bn3MemoryPool::deallocate(ptr);
		Bn3MemoryPool::deallocate(ptr2);
		Bn3MemoryPool::deallocate(ptr3);
		Bn3MemoryPool::deallocate(ptr4);
		Bn3MemoryPool::deallocate(ptr5);
	}


	Bn3MemoryPool::release();
}

void test_dealloc_nullptr(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize(4, 4, 4, 4, 4, 4, 4, 4);
	
	Block<32>* ptr = nullptr;
	Bn3MemoryPool::deallocate(ptr);

	Bn3MemoryPool::release();
}

void test_alloc_and_initialize(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize(4, 4, 4, 4, 4, 4, 4, 4);

	{
		auto* ptr = Bn3MemoryPool::allocate_and_initialize<Block<32>>('a');
		Bn3MemoryPool::deallocate(ptr);
	}
	{
		auto* ptr = Bn3MemoryPool::allocate_and_initialize<Block<64>>('b');
		Bn3MemoryPool::deallocate(ptr);
	}
	{
		auto* ptr = Bn3MemoryPool::allocate_and_initialize<Block<9000>>('c');
		Bn3MemoryPool::deallocate(ptr);
	}

	Bn3MemoryPool::release();
}

void test_alloc_array(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize(4, 4, 4, 4, 4, 4, 4, 4);

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<128>>(5);
		Bn3MemoryPool::deallocate(ptr, 5);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<256>>(6);
		Bn3MemoryPool::deallocate(ptr, 6);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<3000>>(5);
		Bn3MemoryPool::deallocate(ptr, 5);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<1000>>(4);
		Bn3MemoryPool::deallocate(ptr, 4);
	}

	Bn3MemoryPool::release();
}

void test_allocator(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize(4, 4, 4, 4, 4, 4, 4, 4);


	{

		auto ptr = std::shared_ptr<Block<32>>(Bn3MemoryPool::allocate<Block<32>>(1), [](Block<32>* ptr) {
			Bn3MemoryPool::deallocate<Block<32>>(ptr);
			}
		);
		auto ptr2 = ptr;
	}

	{
		auto ptr = std::shared_ptr<Block<64>>(Bn3MemoryPool::allocate<Block<64>>(1), [](Block<64>* ptr) {
			Bn3MemoryPool::deallocate<Block<64>>(ptr);
			}
		);
		auto ptr2 = ptr;
	}

	Bn3MemoryPool::release();
}

void testMemoryPool()
{
	test_allocator(true);
	test_alloc(true);
	test_dealloc_nullptr(true);
	test_alloc_and_initialize(true);
	test_alloc_array(true);
}