#ifndef __BN3MONKEY_STATIC_STRING__
#define __BN3MONKEY_STATIC_STRING__
#include <string>
#include <algorithm>

namespace Bn3Monkey
{
	template<typename T>
	struct is_std_string : std::false_type {};

	template<typename CharT, typename Traits, typename Allocator>
	struct is_std_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

	class Bn3StaticString
	{		

	public:
		static constexpr size_t MAX_LENGTH = 256;
		explicit Bn3StaticString()
		{
		}
		Bn3StaticString(const char* str)
		{
			_length = strlen(str);
			std::copy(str, str + _length, _data);
		}

		template<typename String>
		Bn3StaticString(const String& str)
		{
			static_assert(is_std_string<String>::value, "Input value must be string");
			_length = str.length();
			std::copy(str.begin(), str.end(), _data);
		}
		Bn3StaticString(const Bn3StaticString& other) : _length(other._length)
		{
			std::copy(other._data, other._data + other._length, _data);
		}
		Bn3StaticString(Bn3StaticString&& other) : _length(other._length) {
			std::copy(other._data, other._data + other._length, _data);

			other._length = 0;
			memset(other._data, 0, MAX_LENGTH);
		}

		bool operator==(const char* other)
		{
			return !strncmp(_data, other, MAX_LENGTH);
		}

		template<typename String>
		bool operator==(const String& other)
		{
			static_assert(is_std_string<String>::value, "Input value must be string");
			return !strncmp(_data, other.c_str(), MAX_LENGTH);
		}
		bool operator==(const Bn3StaticString& other)
		{
			return !strncmp(_data, other._data, MAX_LENGTH);
		}


		Bn3StaticString& operator=(const char* other)
		{
			_length = strlen(other);
			memset(_data, 0, MAX_LENGTH);

			std::copy(other, other + _length, _data);
			return *this;
		}

		template<typename String>
		Bn3StaticString& operator=(const String& other)
		{
			static_assert(is_std_string<String>::value, "Input value must be string");
			_length = other.length();
			memset(_data, 0, MAX_LENGTH);
			std::copy(other.begin(), other.end(), _data);
			return *this;
		}

		Bn3StaticString& operator=(const Bn3StaticString& other)
		{
			_length = other._length;
			memset(_data, 0, MAX_LENGTH);

			std::copy(other._data, other._data + other._length, _data);
			return *this;
		}
		
		Bn3StaticString& operator=(Bn3StaticString&& other)
		{
			_length = other._length;
			memset(_data, 0, MAX_LENGTH);

			std::copy(other._data, other._data + other._length, _data);
			return *this;
		}

		Bn3StaticString& operator+=(const char* other)
		{
			size_t length = strlen(other);
			if (length + _length < MAX_LENGTH - 1)
			{
				std::copy(other, other + length, _data + _length);
				_length += length;
			}
			return *this;
		}
		template<typename String>
		Bn3StaticString& operator+=(const String& other)
		{
			static_assert(is_std_string<String>::value, "Input value must be string");
			if (other.length() + _length < MAX_LENGTH - 1)
			{
				std::copy(other.begin(), other.end(), _data + _length);
				_length += other.length();
			}
			return *this;
		}
		Bn3StaticString& operator+=(const Bn3StaticString& other)
		{
			if (other._length + _length < MAX_LENGTH - 1)
			{
				std::copy(other._data, other._data + other._length, _data + _length);
				_length += other._length;
			}
			return *this;
		}


		void push_back(const char& value)
		{
			if (_length < MAX_LENGTH-1)
			{
				_data[_length++] = value;
			}
		}

		const char* data() const { return _data; }


	private:
		size_t _length{ 0 };
		char _data[MAX_LENGTH]{ 0 };
	};
}

#endif // __BN3MONKEY_STATIC_STRING__
