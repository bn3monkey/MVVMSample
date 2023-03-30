#ifndef __TEST_HELPER__
#define __TEST_HELPER__

#include <cstdio>
#include <cstdarg>
#include <cstring>

void say(const char* format, ...)
{
	char buffer[1024]{ 0 };
	va_list args;
	va_start(args, format);
	auto size = vsnprintf(buffer, sizeof(buffer)-1, format, args);
	va_end(args);

	char line[1028]{ 0 };
	memset(line, '=', size + 4);

	printf("%s\n= %s =\n%s\n", line, buffer, line);
}


#endif