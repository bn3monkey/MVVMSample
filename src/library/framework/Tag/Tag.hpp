#ifndef __BN3MONKEY_TAG__
#define __BN3MONKEY_TAG__

#include <cstring>
#include <algorithm>
#include <cassert>

namespace Bn3Monkey
{
	constexpr size_t TAG_SIZE = 16;
	
	struct alignas(16) Bn3Tag
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
			const char* value = other.name;
			size_t length = strlen(value);
			std::copy(value, value + length, name);
			return *this;
		}

		inline void clear() { memset(name, 0, sizeof(TAG_SIZE)); }
		inline const char* str() const { return name; }

	private:
		char name[TAG_SIZE]{ 0 };
	};
}

#endif