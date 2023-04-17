#include <StaticVector/StaticVector.hpp>
#include "../test_helper.hpp"

void testStaticVector(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	Bn3StaticVector<int, 8> a;
	a.push_back(2);
	a.push_back(3);
	a.push_back(4);

	Bn3StaticVector<int, 8> b;
	b.push_back(a);
	b.push_back(5);

	class Sans
	{
	public:
		int a;
		float b;
		Sans(int a, float b) :a(a), b(b) {

		}
		~Sans()
		{
			printf("Oh! No! %d %f\n", a, b);
		}
	};

	{
		Bn3StaticVector<Sans, 10> c;
		c.emplace_back(2, 3.0);
		auto& ret = c[0];
		c.emplace_back(3, 4.0);
		c.emplace_back(4, 5.0);
		auto d = c;
		auto e = std::move(d);
		auto f(d);

	}

	for (auto& value : a)
	{
		printf("%d\n", value);
	}

	return;
}