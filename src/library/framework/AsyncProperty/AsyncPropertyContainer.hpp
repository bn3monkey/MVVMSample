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
			char* ptr = _value + root_len;
			std::copy(root_value, root_value + root_len, _value);
			if (root_len != 0)
			{
				*ptr = '.';
				ptr++;
			}
			std::copy(value, value + strlen(value), ptr);
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
		AsyncPropertyContainer(const Bn3Tag& container_name, const ScopedTaskScope& scope) : _name(container_name), _scope(scope) {
			_nodes = Bn3Map(PropertyPath, AsyncPropertyNode*) { Bn3MapAllocator(PropertyPath, AsyncPropertyNode*, container_name) };
		}
		virtual ~AsyncPropertyContainer() {
			if (_container)
				delete _container;
		}

		bool create(const char* content);

		template<class Type>
		AsyncProperty<Type>* find(const char* path) { 
			auto key = PropertyPath(path);
			auto* value = _nodes.at(key);
			auto* ret = reinterpret_cast<AsyncProperty<Type>*>(value);
			return ret;
		}

		template<class Type, size_t MAX_ARRAY_SIZE>
		AsyncPropertyArray<Type, MAX_ARRAY_SIZE>* findArray(const char* path)
		{
			auto key = PropertyPath(path);
			auto* value = _nodes.at(key);
			auto* ret = reinterpret_cast<AsyncPropertyArray<Type, MAX_ARRAY_SIZE>*>(value);
			return ret;
		}
		void clear() { 
			if (_container)
			{
				delete[] _container;
				_container = nullptr;
			}
			_nodes.clear();

		}


		//          -- onPropertyChanged-->
		//  this                             other
		//         <-- onPropertyNotified--

		virtual void subscribe(AsyncPropertyContainer* other) {}

	private:
		Bn3Monkey::AsyncPropertyExtendedNode getAsyncProperty(char* ptr, const std::string& type, const std::string& name, const nlohmann::json& value);
		Bn3Monkey::AsyncPropertyExtendedNode getAsyncPropertyArray(char* ptr, const std::string& type, const std::string& name, size_t length, const nlohmann::json& values);
		char* assignProperties(char* ptr, const Bn3Monkey::PropertyPath& path, const nlohmann::json& content);

		Bn3Tag _name;

		Bn3Map(PropertyPath, AsyncPropertyNode*) _nodes;
		ScopedTaskScope _scope;
		char* _container;
	};
}

#endif