#include <cstdio>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <functional>
#include <future>
#include <unordered_map>

/*
template<typename Type, size_t MaxSize>
class AsyncArrayProperty
{
public:
	template<size_t N>
	explicit AsyncArrayProperty(const Type(&default_values)[N], size_t size)
	{
		_size = size;
		std::copy(default_values, default_values + size, _values);
	}

	AsyncArrayProperty(const std::initializer_list<Type>& default_values)
	{
		_size = default_values.size();
		std::copy(default_values.begin(), default_values.end(), _values);
	}

	template<size_t size>
	inline bool getElements(Type(&values)[size])
	{
		std::copy(_values, _values + _size, values);
		return true;
	}

	bool set(const Type& value, size_t idx)
	{
		Setter setter;


		auto ret = std::bind(&AsyncArrayProperty::onPropertyProcessed, this, temp, idx, 1);
		ret();
		return true;
	}

	template<size_t size>
	bool setRegion(const Type(&values)[size], size_t offset, size_t length)
	{
		Type temp[MaxSize]{};
		std::copy(values, values + length, temp);

		auto ret = std::bind(&AsyncArrayProperty::onPropertyProcessed, this, temp, offset, length);
		ret();

		return true;
	}

	template<size_t size>
	bool setRegionAsync(const Type(&values)[size], size_t offset, size_t length)
	{
		Type temp[MaxSize]{};
		std::copy(values, values + length, temp);

		auto ret = std::bind(&AsyncArrayProperty::onPropertyProcessed, this, temp, offset, length);
		
		std::thread t(ret);
		t.detach();
		return true;
	}

	void onPropertyProcessed(Setter setter)
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1s);
		std::copy(setter.values, setter.values + setter.length, _values + setter.offset);
	}

private:
	struct Setter
	{
		Type _values[MaxSize];
		size_t _offset;
		size_t _length;

		Setter(const Type(&values)[MaxSize])
		{
			std::copy(values, values + MaxSize, _values)
		}

		void copyFrom(const Type& value, size_t idx)
		{
			_values[idx] = value;
		}
		void copyFrom(const Type(&values)[], size_t offset, size_t length)
		{
			std::copy(values, values + length, _values + offset);
		}
		void copyTo(const Type(&values)[MaxSize])
		{
			std::copy(_values, _values + MaxSize, values);
		}
	};

	Type _values[MaxSize];
	size_t _size;
};
*/


namespace Bn3Monkey
{
	struct PropertyPath
	{
		PropertyPath(const char* value)
		{
			std::copy(value, value + strlen(value), _value);
		}
		PropertyPath(const PropertyPath& root, const char* value)
		{
			const char* root_value = root._value;
			size_t root_len = strlen(root_value);
			std::copy(root_value, root_value + root_len, _value);
			_value[root_len] = '.';
			std::copy(value, value + strlen(value), _value + root_len + 1);
		}
		PropertyPath(const PropertyPath& other)
		{
			std::copy(other._value, other._value + 260, _value);
		}

		PropertyPath(PropertyPath&& other)
		{
			std::copy(other._value, other._value + 260, _value);
		}

		PropertyPath& operator=(const PropertyPath& other)
		{
			std::copy(other._value, other._value + 260, _value);
			return *this;
		}

		bool operator==(const PropertyPath& other) const
		{
			return !strcmp(_value, other._value);
		}

		char _value[260]{ 0 };

	};
}

namespace std {
	template<>
	struct hash<Bn3Monkey::PropertyPath>
	{
		std::size_t operator()(const Bn3Monkey::PropertyPath& value) const {
			return std::hash<std::string>{}(value._value);
		}
	};
}

int main()
{
	/*
	int arr[5] = { 1, 2, 3, 4, 5 };
	
	AsyncArrayProperty<int, 32> a(arr, 5);
	AsyncArrayProperty<int, 32> b({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

	int arr2[10] = { 0, };
	b.getElements(arr2);

	a.set(3, 2);

	int arr3[3] = { 120 ,130, 140 };
	b.setRegion(arr3, 6, 3);
	b.setRegionAsync(arr3, 2, 3);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(5s);
	return 0;
	*/

	

	int a = 1;
	int b = 2;
	int c = 3;
	int d = 4;
	int e = 5;

	int a_b = 7;
	int a_b_d = 10;

	using namespace Bn3Monkey;
	std::unordered_map<PropertyPath, int*> sans;

	sans["apple"] = &a;
	sans[PropertyPath(PropertyPath("apple"), "banana")] = &a_b;
	sans["banana"] = &b;
	sans["cde"] = &c;
	sans["dest"] = &d;
	sans["egg"] = &e;
	sans[PropertyPath(PropertyPath(PropertyPath("apple"), "banana"), "death")] = &a_b_d;

	{
		int* value = sans["apple"];
		value = sans["apple.banana"];
		value = sans["banana"];
		value = sans["cde"];
		value = sans["dest"];
		value = sans["egg"];
		value = sans["apple.banana.death"];
		printf("%d\n", *value);
	}
}