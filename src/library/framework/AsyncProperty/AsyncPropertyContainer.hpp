#ifndef __BN3MONKEY_ASYNC_PROPERTY_CONTAINER__
#define __BN3MONKEY_ASYNC_PROPERTY_CONTAINER__

#include "AsyncPropertyImpl.hpp"
#include "AsyncPropertyArray.hpp"

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

		std::size_t operator()() const {
			return std::hash<std::string>{}(_value);
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


namespace Bn3Monkey
{
	class AsyncPropertyContainer
	{
	public:

		template<class Property, class PropertyArray>
		void create(const char* content) {}

		template<class Type>
		AsyncProperty<Type>* find(const char* position) { return nullptr;  }

		template<class Type, size_t MAX_ARRAY_SIZE>
		AsyncPropertyArray<Type, MAX_ARRAY_SIZE>* findArray(const char* position) { return nullptr; }

		void clear() { return; }


		//          -- onPropertyChanged-->
		//  this                             other
		//         <-- onPropertyNotified--

		virtual void subscribe(AsyncPropertyContainer* other) {}

	private:
		size_t getPropertiesSize() { return 0; }
		
		AsyncPropertyNode* assignProperty(const Bn3Tag& name) { return nullptr; }

		Bn3Map(PropertyPath, AsyncPropertyNode*) _nodes;
		char* _container;
	};
}

#endif