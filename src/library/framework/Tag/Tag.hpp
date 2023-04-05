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
			assert(length < TAG_SIZE - 1);
			std::copy(value, value + length, name);
		}
		explicit Bn3Tag(const Bn3Tag& prefix, const char* value)
		{
			size_t prefix_len = strlen(prefix.name);
			size_t length = strlen(value);
			assert(prefix_len + length < TAG_SIZE - 1);

			std::copy(prefix.name, prefix.name + prefix_len, name);
			std::copy(value, value+ length, name + prefix_len);
		}
		explicit Bn3Tag(const char* value, const Bn3Tag& postfix)
		{
			size_t postfix_len = strlen(postfix.name);
			size_t length = strlen(value);
			assert(postfix_len + length < TAG_SIZE - 1);

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

			printf("Double Access check : %s (%p)\n", other.name, name);
			for (size_t i = 0; i < length; i++)
			{
				printf("Copy %d index (%p)\n", i, name);
				if (i != length - 1)
				{
					if (name[i + 1] != 0)
					{
						printf("<%p> Check!\n", name);
					}
				}
				name[i] = other.name[i];
			}

			return *this;
		}

		inline bool operator==(const Bn3Tag& other)
		{
			return !strcmp(name, other.name);
		}

		inline void clear() { memset(name, 0, sizeof(TAG_SIZE)); }
		inline const char* str() const { return name; }

	private:
		volatile char name[TAG_SIZE]{ 0 };
	};
}

#endif