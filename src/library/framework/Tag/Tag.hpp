#ifndef __BN3MONKEY_TAG__
#define __BN3MONKEY_TAG__

#include <cstring>
#include <algorithm>
#include <cassert>

namespace Bn3Monkey
{
	constexpr size_t TAG_SIZE = 32;
	
	struct Bn3Tag
	{
		Bn3Tag() {
			// name is 0
		}
		explicit Bn3Tag(const char* value)
		{
			size_t length = strlen(value);
			if (length >= TAG_SIZE - 1)
				length = TAG_SIZE - 1;
			std::copy(value, value + length, name);
		}
		explicit Bn3Tag(const Bn3Tag& prefix, const char* value)
		{
			size_t prefix_len = strlen(prefix.name);
			size_t length = strlen(value);
			if (prefix_len + length >= TAG_SIZE - 1)
			{
				length = TAG_SIZE - 1 - prefix_len;
			}

			std::copy(prefix.name, prefix.name + prefix_len, name);
			std::copy(value, value+ length, name + prefix_len);
		}
		explicit Bn3Tag(const char* value, const Bn3Tag& postfix)
		{
			size_t postfix_len = strlen(postfix.name);
			size_t length = strlen(value);
			if (postfix_len + length < TAG_SIZE - 1)
			{
				length = TAG_SIZE - 1 - postfix_len;
			}

			std::copy(value, value + length, name);
			std::copy(postfix.name, postfix.name + postfix_len, name + length);
		}
		Bn3Tag(const Bn3Tag& other)
		{
			const char* value = other.name;
			size_t length = strlen(value);
			std::copy(value, value + length, name);
		}
		Bn3Tag(Bn3Tag&& other) noexcept
		{
			const char* value = other.name;
			size_t length = strlen(value);
			std::copy(value, value + length, name);
		}
		Bn3Tag& operator=(const Bn3Tag& other)
		{
			clear();
			const char* value = other.name;
			size_t length = strlen(value);
			std::copy(value, value + length, name);

			return *this;
		}

		inline bool operator==(const Bn3Tag& other)
		{
			return !strcmp(name, other.name);
		}

		inline void clear() { memset(name, 0, sizeof(name)); }
		inline const char* str() const { return name; }

	private:
		char name[TAG_SIZE]{ 0 };
	};
}

#endif