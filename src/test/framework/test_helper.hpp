#ifndef __GLOBAL_TEST_HELPER__
#define __GLOBAL_TEST_HELPER__

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <MemoryPool/MemoryPool.hpp>

inline void say(const char* format, ...)
{
	char buffer[1024]{ 0 };
	va_list args;
	va_start(args, format);
	auto size = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	char line[1028]{ 0 };
	memset(line, '=', size + 4);

	printf("%s\n= %s =\n%s\n", line, buffer, line);
}

extern Bn3Monkey::Bn3MemoryPool::Analyzer analyzer;

#endif