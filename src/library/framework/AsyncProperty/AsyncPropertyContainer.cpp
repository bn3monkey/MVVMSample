#include "AsyncPropertyContainer.hpp"

#include "json.hpp"

template<size_t MAX_ARRAY_SIZE>
size_t getAsyncPropertyArraySizeImpl(const std::string& type)
{
	size_t type_size{ 0 };
	if (type == "bool")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<bool, MAX_ARRAY_SIZE>);
	else if (type == "int8_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<int8_t, MAX_ARRAY_SIZE>);
	else if (type == "int16_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<int16_t, MAX_ARRAY_SIZE>);
	else if (type == "int32_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<int32_t, MAX_ARRAY_SIZE>);
	else if (type == "int64_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<int64_t, MAX_ARRAY_SIZE>);
	else if (type == "uint8_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint8_t, MAX_ARRAY_SIZE>);
	else if (type == "uint16_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint16_t, MAX_ARRAY_SIZE>);
	else if (type == "uint32_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint32_t, MAX_ARRAY_SIZE>);
	else if (type == "uint64_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint64_t, MAX_ARRAY_SIZE>);
	else if (type == "float")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<float, MAX_ARRAY_SIZE>);
	else if (type == "double")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<double, MAX_ARRAY_SIZE>);
	else if (type == "std::string")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<Bn3Monkey::Bn3StaticString, MAX_ARRAY_SIZE>);
	return type_size;
}

template<size_t MAX_ARRAY_SIZE>
static size_t getAsyncPropertyArraySizeImplRecursive(size_t length, const std::string& type)
{
	if (length <= MAX_ARRAY_SIZE)
		return getAsyncPropertyArraySizeImpl<MAX_ARRAY_SIZE>(type);
	return getAsyncPropertyArraySizeImplRecursive<MAX_ARRAY_SIZE * 2>(length, type);
}

template<>
static size_t getAsyncPropertyArraySizeImplRecursive<512>(size_t max_length, const std::string& type)
{
	return getAsyncPropertyArraySizeImpl<512>(type);
}

size_t getAsyncPropertyArraySize(size_t length, const std::string& type)
{
	return getAsyncPropertyArraySizeImplRecursive<2>(length, type);
}

size_t getAsyncPropertySize(const std::string& type)
{
	size_t type_size{ 0 };
	if (type == "bool")
		type_size = sizeof(Bn3Monkey::AsyncProperty<bool>);
	else if (type == "int8_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<int8_t>);
	else if (type == "int16_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<int16_t>);
	else if (type == "int32_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<int32_t>);
	else if (type == "int64_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<int64_t>);
	else if (type == "uint8_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint8_t>);
	else if (type == "uint16_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint16_t>);
	else if (type == "uint32_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint32_t>);
	else if (type == "uint64_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint64_t>);
	else if (type == "float")
		type_size = sizeof(Bn3Monkey::AsyncProperty<float>);
	else if (type == "double")
		type_size = sizeof(Bn3Monkey::AsyncProperty<double>);
	else if (type == "std::string")
		type_size = sizeof(Bn3Monkey::AsyncProperty<Bn3Monkey::Bn3StaticString>);
	return type_size;
}


static size_t getPropertiesSize(const nlohmann::json& content)
{
	size_t node_size {0};

	if (content.contains("values"))
	{
		do
		{
			auto type_iter = content.find("type");
			if (type_iter == content.end())
			{
				break;
			}
			auto length_iter = content.find("length");
			if (length_iter == content.end())
			{
				break;
			}

			auto& type_node = *type_iter;
			auto& length_node = *length_iter;

			auto& type = type_node.get_ref<const std::string&>();
			
			size_t length = length_node.get<size_t>();

			size_t type_size{ 0 };
			
			if (length == 1)
			{
				node_size = getAsyncPropertySize(type);
			}
			else {
				node_size = getAsyncPropertyArraySize(length, type);
			}
		} while (false);
	}

	auto childs_iter = content.find("childs");
	if (childs_iter != content.end())
	{
		auto& childs = *childs_iter;
		for (auto& child : childs)
		{
			node_size += getPropertiesSize(child);
		}
	}
	
	return node_size;
}

template<typename T>
static Bn3Monkey::AsyncPropertyExtendedNode allocateProperty(char* ptr, const std::string& name, Bn3Monkey::ScopedTaskScope& scope, const nlohmann::json& value)
{
	Bn3Monkey::AsyncPropertyExtendedNode ret;
	T default_value{ value[0].get<T>()};
	auto* node_t =  new(ptr) Bn3Monkey::AsyncProperty<T>(Bn3Monkey::Bn3Tag(name.c_str()), scope, default_value);
	ret.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
	ret.size =  sizeof(Bn3Monkey::AsyncProperty<T>);
	return ret;
}

template<>
static Bn3Monkey::AsyncPropertyExtendedNode allocateProperty<Bn3Monkey::Bn3StaticString>(char* ptr, const std::string& name, Bn3Monkey::ScopedTaskScope& scope, const nlohmann::json& value)
{
	Bn3Monkey::AsyncPropertyExtendedNode ret;
	Bn3Monkey::Bn3StaticString default_value{ value[0].get_ref<const std::string&>().c_str()};
	auto* node_t = new(ptr) Bn3Monkey::AsyncProperty<Bn3Monkey::Bn3StaticString>(Bn3Monkey::Bn3Tag(name.c_str()), scope, default_value);
	ret.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
	ret.size = sizeof(Bn3Monkey::AsyncProperty<Bn3Monkey::Bn3StaticString>);
	return ret;
}

Bn3Monkey::AsyncPropertyExtendedNode Bn3Monkey::AsyncPropertyContainer::getAsyncProperty(char* ptr, const std::string& type, const std::string& name, const nlohmann::json& value)
{
	Bn3Monkey::AsyncPropertyExtendedNode node;

	if (type == "bool")
	{
		node = allocateProperty<bool>(ptr, name, _scope, value);
	}
	else if (type == "int8_t")
	{
		node = allocateProperty<int8_t>(ptr, name, _scope, value);
	}
	else if (type == "int16_t")
	{
		node = allocateProperty<int16_t>(ptr, name, _scope, value);
	}
	else if (type == "int32_t")
	{
		node = allocateProperty<int32_t>(ptr, name, _scope, value);
	}
	else if (type == "int64_t")
	{
		node = allocateProperty<int64_t>(ptr, name, _scope, value);
	}
	else if (type == "uint8_t")
	{
		node = allocateProperty<uint8_t>(ptr, name, _scope, value);
	}
	else if (type == "uint16_t")
	{
		node = allocateProperty<uint16_t>(ptr, name, _scope, value);
	}
	else if (type == "uint32_t")
	{
		node = allocateProperty<uint32_t>(ptr, name, _scope, value);
	}
	else if (type == "uint64_t")
	{
		node = allocateProperty<uint64_t>(ptr, name, _scope, value);
	}
	else if (type == "float")
	{
		node = allocateProperty<float>(ptr, name, _scope, value);
	}
	else if (type == "double")
	{
		node = allocateProperty<double>(ptr, name, _scope, value);
	}
	else if (type == "std::string")
	{
		node = allocateProperty<Bn3Monkey::Bn3StaticString>(ptr, name, _scope, value);
	}

	return node;
}

template<typename T>
class AsyncPropertyArrayGetter
{
public:
	template<size_t MAX_ARRAY_SIZE>
	static Bn3Monkey::AsyncPropertyExtendedNode getRecursive(char* ptr, const std::string& name, const Bn3Monkey::ScopedTaskScope& scope, size_t length, const nlohmann::json& values)
	{
		Bn3Monkey::AsyncPropertyExtendedNode node;
		if (length <= MAX_ARRAY_SIZE)
		{
			T temp_values[MAX_ARRAY_SIZE];
			size_t i = 0;
			for (auto& value : values)
			{
				temp_values[i++] = value.get<T>();
			}

			auto* node_t = new(ptr) Bn3Monkey::AsyncPropertyArray<T, MAX_ARRAY_SIZE>(Bn3Monkey::Bn3Tag(name.c_str()), scope, temp_values, length);
			node.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
			node.size = sizeof(Bn3Monkey::AsyncPropertyArray<T, MAX_ARRAY_SIZE>);
			return node;
		}
		return AsyncPropertyArrayGetter<T>::getRecursive<MAX_ARRAY_SIZE * 2>(ptr, name, scope, length, values);
	}

	template<>
	static Bn3Monkey::AsyncPropertyExtendedNode getRecursive<512llu>(char* ptr, const std::string& name, const Bn3Monkey::ScopedTaskScope& scope, size_t length, const nlohmann::json& values)
	{
		Bn3Monkey::AsyncPropertyExtendedNode node;

		T temp_values[512llu];
		size_t i = 0;
		for (auto& value : values)
		{
			temp_values[i++] = value.get<T>();
		}

		auto* node_t = new(ptr) Bn3Monkey::AsyncPropertyArray<T, 512llu>(Bn3Monkey::Bn3Tag(name.c_str()), scope, temp_values, length);
		node.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
		node.size = sizeof(Bn3Monkey::AsyncPropertyArray<T, 512llu>);
		return node;
	}
};

template<>
class AsyncPropertyArrayGetter<Bn3Monkey::Bn3StaticString>
{
public:
	template<size_t MAX_ARRAY_SIZE>
	static Bn3Monkey::AsyncPropertyExtendedNode getRecursive(char* ptr, const std::string& name, const Bn3Monkey::ScopedTaskScope& scope, size_t length, const nlohmann::json& values)
	{
		Bn3Monkey::AsyncPropertyExtendedNode node;
		if (length <= MAX_ARRAY_SIZE)
		{
			Bn3Monkey::Bn3StaticString temp_values[MAX_ARRAY_SIZE];
			size_t i = 0;
			for (auto& value : values)
			{
				temp_values[i++] = Bn3Monkey::Bn3StaticString (value.get_ref<const std::string&>());
			}

			auto* node_t = new(ptr) Bn3Monkey::AsyncPropertyArray<Bn3Monkey::Bn3StaticString, MAX_ARRAY_SIZE>(Bn3Monkey::Bn3Tag(name.c_str()), scope, temp_values, length);
			node.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
			node.size = sizeof(Bn3Monkey::AsyncPropertyArray<Bn3Monkey::Bn3StaticString, MAX_ARRAY_SIZE>);
		}
		return AsyncPropertyArrayGetter<Bn3Monkey::Bn3StaticString>::getRecursive<MAX_ARRAY_SIZE * 2>(ptr, name, scope, length, values);
	}

	template<>
	static Bn3Monkey::AsyncPropertyExtendedNode getRecursive<512llu>(char* ptr, const std::string& name, const Bn3Monkey::ScopedTaskScope& scope, size_t length, const nlohmann::json& values)
	{
		Bn3Monkey::AsyncPropertyExtendedNode node;

		Bn3Monkey::Bn3StaticString temp_values[512llu];
		size_t i = 0;
		for (auto& value : values)
		{
			temp_values[i++] = Bn3Monkey::Bn3StaticString(value.get_ref<const std::string&>());
		}

		auto* node_t = new(ptr) Bn3Monkey::AsyncPropertyArray<Bn3Monkey::Bn3StaticString, 512llu>(Bn3Monkey::Bn3Tag(name.c_str()), scope, temp_values, length);
		node.node = reinterpret_cast<Bn3Monkey::AsyncPropertyNode*>(node_t);
		node.size = sizeof(Bn3Monkey::AsyncPropertyArray<Bn3Monkey::Bn3StaticString, 512llu>);
		return node;
	}
};

template<typename T>
static Bn3Monkey::AsyncPropertyExtendedNode allocatePropertyArray(char* ptr, const std::string& name, Bn3Monkey::ScopedTaskScope& scope, size_t length, const nlohmann::json& values)
{
	Bn3Monkey::AsyncPropertyExtendedNode ret;
	ret = AsyncPropertyArrayGetter<T>::getRecursive<2>(ptr, name, scope, length, values);
	return ret;
}

Bn3Monkey::AsyncPropertyExtendedNode Bn3Monkey::AsyncPropertyContainer::getAsyncPropertyArray(char* ptr, const std::string& type, const std::string& name, size_t length, const nlohmann::json& values)
{
	Bn3Monkey::AsyncPropertyExtendedNode node;

	if (type == "bool")
	{
		node = allocatePropertyArray<bool>(ptr, name, _scope, length, values);
	}
	else if (type == "int8_t")
	{
		node = allocatePropertyArray<int8_t>(ptr, name, _scope, length, values);
	}
	else if (type == "int16_t")
	{
		node = allocatePropertyArray<int16_t>(ptr, name, _scope, length, values);
	}
	else if (type == "int32_t")
	{
		node = allocatePropertyArray<int32_t>(ptr, name, _scope, length, values);
	}
	else if (type == "int64_t")
	{
		node = allocatePropertyArray<int64_t>(ptr, name, _scope, length, values);
	}
	else if (type == "uint8_t")
	{
		node = allocatePropertyArray<uint8_t>(ptr, name, _scope, length, values);
	}
	else if (type == "uint16_t")
	{
		node = allocatePropertyArray<uint16_t>(ptr, name, _scope, length, values);
	}
	else if (type == "uint32_t")
	{
		node = allocatePropertyArray<uint32_t>(ptr, name, _scope, length, values);
	}
	else if (type == "uint64_t")
	{
		node = allocatePropertyArray<uint64_t>(ptr, name, _scope, length, values);
	}
	else if (type == "float")
	{
		node = allocatePropertyArray<float>(ptr, name, _scope, length, values);
	}
	else if (type == "double")
	{
		node = allocatePropertyArray<double>(ptr, name, _scope, length, values);
	}
	else if (type == "std::string")
	{
		node = allocatePropertyArray<Bn3Monkey::Bn3StaticString>(ptr, name, _scope, length, values);
	}

	return node;
}

char* Bn3Monkey::AsyncPropertyContainer::assignProperties(char* ptr, const Bn3Monkey::PropertyPath& path, const nlohmann::json& content)
{
	auto name_iter = content.find("name");
	if (name_iter == content.end())
	{
		return ptr;
	}

	auto& name_node = *name_iter;
	auto& name = name_node.get_ref<const std::string&>();
	auto current_path = PropertyPath(path, name.c_str());	

	do
	{
		auto value_iter = content.find("values");
		if (value_iter == content.end())
			break;

		auto type_iter = content.find("type");
		if (type_iter == content.end())
			break;


		auto length_iter = content.find("length");
		if (length_iter == content.end())
			break;

		auto& type_node = *type_iter;
		auto& length_node = *length_iter;

		auto& value = *value_iter;
		auto& type = type_node.get_ref<const std::string&>();
		size_t length = length_node.get<size_t>();

		Bn3Monkey::AsyncPropertyExtendedNode node;
		if (length == 1)
		{
			node = getAsyncProperty(ptr, type, name, value);
		}
		else
		{
			node = getAsyncPropertyArray(ptr, type, name, length, value);
		}

		_nodes[current_path] = node.node;
		ptr += node.size;

	} while (false);

	auto childs_iter = content.find("childs");
	if (childs_iter != content.end())
	{
		auto& childs = *childs_iter;
		for (auto& child : childs)
		{
			ptr = assignProperties(ptr, current_path, child);
		}
	}

	return ptr;
}

bool Bn3Monkey::AsyncPropertyContainer::create(const char* content)
{
	auto json_content = nlohmann::json::parse(content);
	size_t content_length = getPropertiesSize(json_content);

	_container = Bn3Monkey::Bn3MemoryPool::allocate<char>(_name, content_length);

	char* ptr = _container;
	auto childs_iter = json_content.find("childs");
	if (childs_iter != json_content.end())
	{
		auto& childs = *childs_iter;
		for (auto& child : childs)
		{
			ptr = assignProperties(ptr, PropertyPath(""), child);
		}
	}

	printf("%llu\n", content_length);
	return true;
}