#ifndef __BN3MONKEY_ASYNC_PROPERTY_CONTAINER__
#define __BN3MONKEY_ASYNC_PROPERTY_CONTAINER__

#include "AsyncPropertyImpl.hpp"
#include "AsyncPropertyArray.hpp"
#include "json.hpp"

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
		AsyncPropertyContainer(const Bn3Tag& container_name, const ScopedTaskScope& scope) : _name(container_name) {}
		virtual ~AsyncPropertyContainer() {
			if (_container)
				delete _container;
		}

		bool create(const char* content);

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
		void mapAsyncProperty(const PropertyPath& path, AsyncPropertyNode* node);
		char* getAsyncProperty(char* ptr, const std::string& type, const Bn3Monkey::PropertyPath& path, const nlohmann::json& content);
		char* getAsyncPropertyArray(char* ptr, const std::string& type, const Bn3Monkey::PropertyPath& path, const nlohmann::json& content);
		char* assignProperties(const Bn3Monkey::PropertyPath& path, char* allocated_ptr, const nlohmann::json& content);

		Bn3Tag _name;

		Bn3Map(PropertyPath, AsyncPropertyNode*) _nodes;
		char* _container;
	};
}

#endif