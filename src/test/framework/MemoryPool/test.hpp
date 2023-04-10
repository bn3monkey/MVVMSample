#include <MemoryPool/MemoryPool.hpp>

#include "test_helper.hpp"
#include "../test_helper.hpp"


constexpr size_t block_sizes[] = { 64, 128, 256, 512, 1024, 2048, 4098, 8192 };
constexpr size_t block_sizes_length = sizeof(block_sizes) / sizeof(size_t);

template<size_t idx>
void inner_test_alloc(BlockBase* (&ptr)[block_sizes_length][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[idx];

	ptr[idx][0] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p0"));
	ptr[idx][1] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p1"));
	ptr[idx][2] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p2"));
	ptr[idx][3] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p3"));

	ptr[idx][4] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p4"));
	if (ptr[idx][4] == nullptr)
	{
		say("Good! (Allocate Full check)");
	}
	
	inner_test_alloc<idx - 1>(ptr);
}
template<>
void inner_test_alloc<0>(BlockBase* (&ptr)[block_sizes_length][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[0];

	ptr[0][0] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p0"));
	ptr[0][1] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p1"));
	ptr[0][2] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p2"));
	ptr[0][3] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p3"));

	ptr[0][4] = Bn3MemoryPool::construct < Block<block_size> >(Bn3Tag("p4"));
	if (ptr[0][4] == nullptr)
	{
		say("Good! (Allocate Full check)");
	}
}

template<size_t idx>
void inner_test_dealloc(BlockBase* (&ptr)[block_sizes_length][5])
{
	using namespace Bn3Monkey;
	constexpr size_t block_size = block_sizes[idx];

	Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][0]);
	ptr[idx][4] = Bn3MemoryPool::construct < Block<block_size >>(Bn3Tag("newp4"));

	Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][1]);
	Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][2]);
	Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][3]);
	Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][4]);

	if (!Bn3MemoryPool::destroy((Block<block_size> *)ptr[idx][4]))
	{
		say("Good! (Double Free check)");
	}

	inner_test_dealloc<idx - 1>(ptr);
}
template<>
void inner_test_dealloc<0>(BlockBase* (&ptr)[block_sizes_length][5])
{
	using namespace Bn3Monkey;
	
	constexpr size_t block_size = block_sizes[0];

	Bn3MemoryPool::destroy(ptr[0][0]);
	ptr[0][4] = Bn3MemoryPool::construct < Block<block_size >>(Bn3Tag("newp4"));

	Bn3MemoryPool::destroy(ptr[0][1]);
	Bn3MemoryPool::destroy(ptr[0][2]);
	Bn3MemoryPool::destroy(ptr[0][3]);
	Bn3MemoryPool::destroy(ptr[0][4]);

	if (!Bn3MemoryPool::destroy(ptr[0][4]))
	{
		say("Good! (Double Free check)");
	}
}

void test_alloc(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize({ 4, 4, 4, 4, 4, 4, 4, 4 });
	BlockBase* ptr[block_sizes_length][5];

	inner_test_alloc<block_sizes_length-1>(ptr);
	inner_test_dealloc<block_sizes_length - 1>(ptr);

	{
		auto* ptr = Bn3MemoryPool::construct < Block<10000 >> (Bn3Tag("big1"));
		auto* ptr2 = Bn3MemoryPool::construct < Block<10000 >> (Bn3Tag("big2"));
		auto * ptr3 = Bn3MemoryPool::construct < Block<10000 >>(Bn3Tag("big3"));
		auto * ptr4 = Bn3MemoryPool::construct < Block<10000 >>(Bn3Tag("big4"));
		auto * ptr5 = Bn3MemoryPool::construct < Block<10000 >>(Bn3Tag("big5"));

		Bn3MemoryPool::destroy(ptr);
		Bn3MemoryPool::destroy(ptr2);
		Bn3MemoryPool::destroy(ptr3);
		Bn3MemoryPool::destroy(ptr4);
		Bn3MemoryPool::destroy(ptr5);
	}


	Bn3MemoryPool::release();
}

void test_dealloc_nullptr(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize({ 4, 4, 4, 4, 4, 4, 4, 4 });
	
	Block<64>* ptr = nullptr;
	Bn3MemoryPool::destroy(ptr);


	
	Bn3MemoryPool::release();
}

void test_alloc_and_initialize(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize({ 4, 4, 4, 4, 4, 4, 4, 4 });
	
	{
		auto* ptr = Bn3MemoryPool::construct<Block<64>>(Bn3Tag("a_ptr"), 'a');
		Bn3MemoryPool::destroy(ptr);
	}
	{
		auto* ptr = Bn3MemoryPool::construct<Block<128>>(Bn3Tag("b_ptr"), 'b');
		Bn3MemoryPool::destroy(ptr);
	}
	{
		auto* ptr = Bn3MemoryPool::construct<Block<256>>(Bn3Tag("c_ptr"), 'c');
		Bn3MemoryPool::destroy(ptr);
	}
	{
		auto* ptr = Bn3MemoryPool::construct<Block<9000>>(Bn3Tag("d_ptr"), 'd');
		Bn3MemoryPool::destroy(ptr);
	}

	
	Bn3MemoryPool::release();
}

void test_alloc_array(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize({ 4, 4, 4, 4, 4, 4, 4, 4 });

	
	{
		auto* ptr = Bn3MemoryPool::allocate<Block<128>>(Bn3Tag("128"), 5);
		Bn3MemoryPool::deallocate(ptr, 5);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<256>>(Bn3Tag("256"), 6);
		Bn3MemoryPool::deallocate(ptr, 6);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<3000>>(Bn3Tag("3000"), 5);
		Bn3MemoryPool::deallocate(ptr, 5);
	}

	{
		auto* ptr = Bn3MemoryPool::allocate<Block<1000>>(Bn3Tag("1000"), 4);
		Bn3MemoryPool::deallocate(ptr, 4);
	}

	
	Bn3MemoryPool::release();
}

void test_sharedPtr(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize({ 4, 4, 4, 4, 4, 4, 4, 4 });
	

	{
		auto ptr = makeSharedFromMemoryPool<Block<64>>(Bn3Tag("64"));
		auto ptr2 = ptr;
	}


	{
		auto ptr = makeSharedFromMemoryPool<Block<128>>(Bn3Tag("128"));
		auto ptr2 = ptr;
	}

	
	Bn3MemoryPool::release();
}

void test_allocator(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3MemoryPool::initialize( { 256, 256, 4, 4, 4, 4, 4, 4 });

	{
		std::vector<int> sans(Bn3Allocator<int>(Bn3Tag("sans")));
		for (int i = 0; i < 256; i++)
			sans.push_back(i);
	}


	{
		Bn3Container::string tt{Bn3Allocator<char>(Bn3Tag("tt"))};
		tt = "abcd";
		tt = "abcdefghijklmnlakshdfklahsdfklahlksdfhlkasdhfklahsdfklahsdklfhaklsdfhlaksdhklasdhflkasdh";
		tt.clear();
		tt = "sans!!";
		tt.append("papyrus!!");
	}

	
	{
		Bn3Container::vector<int> sans{Bn3Allocator<int>(Bn3Tag("sans"))};

		sans.resize(15);
		sans[0] = 1;
		sans[1] = 2;
		sans[2] = 3;
		sans[3] = 4;
		int a = sans[0];
		int b = sans[1];
	}
	{
		Bn3Container::map<int, int> papyrus{ Bn3Allocator<std::pair<const int, int>>(Bn3Tag("papyrus")) };
		for (int i = 0; i < 128; i++)
		{
			papyrus[i] = i;
		}

		Bn3Container::queue<Block<64>> undyne { Bn3Allocator<Block<64>>(Bn3Tag("undyne"))};
		{
			Block<64> a;
			undyne.push(a);
			undyne.push(a);
			undyne.push(a);
			undyne.push(a);
			undyne.pop();
			undyne.pop();
			undyne.pop();
			undyne.pop();
		}
	}
	

	Bn3MemoryPool::release();
}

void testMemoryPool(bool value)
{
	if (!value)
		return;
	test_allocator(true);
	test_sharedPtr(true);
	test_alloc(true);
	test_dealloc_nullptr(true);
	test_alloc_and_initialize(true);
	test_alloc_array(true);
}