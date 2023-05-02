#include <StaticString/StaticString.hpp>
#include <MemoryPool/MemoryPool.hpp>
#include "../test_helper.hpp"

void testStaticString(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3StaticString str;
	printf("%s\n", str.data());

	Bn3StaticString str1{ "a" };
	printf("%s\n", str1.data());
	Bn3StaticString str2{ std::string("b")};
	printf("%s\n", str2.data());

	str = "abc";
	printf("%s\n", str.data());
	str = std::string("def");
	printf("%s\n", str.data());

	str1 += "bc";
	printf("%s\n", str1.data());
	str2 += std::string("cd");
	printf("%s\n", str2.data());

	str = str1;
	printf("%s\n", str.data());
	str += str2;
	printf("%s\n", str.data());

	Bn3MemoryPool::initialize({ 256, 256, 4, 4, 4, 4, 4, 4, 4 });
	Bn3Container::string tt{ "defg" };
	str += tt;
	printf("%s\n", str.data());
	Bn3MemoryPool::release();
}