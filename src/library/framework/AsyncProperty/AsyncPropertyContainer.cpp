#include "AsyncPropertyContainer.hpp"

#include "json.hpp"

size_t getArraySize(size_t length)
{
	size_t ret {2};
	if ((length & (length - 1)) == 0)
	{
		return length;
	}
	else {
		while (length < ret)
		{
			ret <<= 1;
		}
	}
	return ret;
}

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
	else if (type == "uint8_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint8_t, MAX_ARRAY_SIZE>);
	else if (type == "uint16_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint16_t, MAX_ARRAY_SIZE>);
	else if (type == "uint32_t")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<uint32_t, MAX_ARRAY_SIZE>);
	else if (type == "float")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<float, MAX_ARRAY_SIZE>);
	else if (type == "double")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<double, MAX_ARRAY_SIZE>);
	else if (type == "std::string")
		type_size = sizeof(Bn3Monkey::AsyncPropertyArray<std::string, MAX_ARRAY_SIZE>);
	return type_size;
}

template<size_t MAX_ARRAY_SIZE>
static constexpr size_t getAsyncPropertyArraySizeImplRecursive(size_t max_length, const std::string& type)
{
	if (max_length == MAX_ARRAY_SIZE)
		return getAsyncPropertyArraySizeImpl<MAX_ARRAY_SIZE>(type);
	return getAsyncPropertyArraySizeImpl<MAX_ARRAY_SIZE / 2>(type);
}

template<>
static constexpr size_t getAsyncPropertyArraySizeImplRecursive<2>(size_t max_length, const std::string& type)
{
	if (max_length == 2)
		return getAsyncPropertyArraySizeImpl<2>(type);
	return 0;
}

size_t getAsyncPropertyArraySize(size_t length, const std::string& type)
{
	size_t max_length = getArraySize(length);
	return getAsyncPropertyArraySizeImplRecursive<512>(length, type);
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
	else if (type == "uint8_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint8_t>);
	else if (type == "uint16_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint16_t>);
	else if (type == "uint32_t")
		type_size = sizeof(Bn3Monkey::AsyncProperty<uint32_t>);
	else if (type == "float")
		type_size = sizeof(Bn3Monkey::AsyncProperty<float>);
	else if (type == "double")
		type_size = sizeof(Bn3Monkey::AsyncProperty<double>);
	else if (type == "std::string")
		type_size = sizeof(Bn3Monkey::AsyncProperty<std::string>);
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
				type_size = getAsyncPropertySize(type);
			}
			else {
				type_size = getAsyncPropertyArraySize(length, type);
			}
			
			
			node_size = length * type_size;

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


void Bn3Monkey::AsyncPropertyContainer::mapAsyncProperty(const PropertyPath& path, AsyncPropertyNode* node)
{
	_nodes[path] = node;
}
char* Bn3Monkey::AsyncPropertyContainer::getAsyncProperty(char* ptr, const std::string& type, const Bn3Monkey::PropertyPath& path, const nlohmann::json& content)
{
	auto value_iter = content.fi
}

static char* assignProperties(Bn3Map(Bn3Monkey::PropertyPath, Bn3Monkey::AsyncPropertyNode*)& nodes, const Bn3Monkey::PropertyPath& path, char* allocated_ptr, const nlohmann::json& content)
{
	char* ptr = allocated_ptr;

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
				type_size = getAsyncPropertySize(type);
			}
			else {
				type_size = getAsyncPropertyArraySize(length, type);
			}


			node_size = length * type_size;

		} while (false);
	}

	auto childs_iter = content.find("childs");
	if (childs_iter != content.end())
	{
		auto& childs = *childs_iter;
		for (auto& child : childs)
		{
			ptr = assignProperties(nodes, child_path, ptr, child);
		}
	}

	return ptr;
}



bool Bn3Monkey::AsyncPropertyContainer::create(const char* content)
{
	auto json_content = nlohmann::json::parse(content);
	size_t content_length = getPropertiesSize(json_content);

	_container = Bn3Monkey::Bn3MemoryPool::allocate<char>(_name, content_length);

	PropertyPath root_path("");
	assignProperties(_nodes, root_path, _container, json_content);

	printf("%llu\n", content_length);
	return true;
}