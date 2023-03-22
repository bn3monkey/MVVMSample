#ifndef __BN3MONKEY_LOG__
#define __BN3MONKEY_LOG__

#include <cstdarg>
#include <cstdio>
#include <string>
#include <mutex>

#if defined _WIN32 //WINDOWS
#include <Windows.h>
#elif defined __ANDROID__ //ANDROID
#include <android/log.h>
#elif (defined __linux__) && !(defined __ANDROID__) // LINUX
#include <cstdio>
#endif

namespace Bn3Monkey
{
    class Log
    {
    public:
        static const int PRIO_VERBOSE;
        static const int PRIO_DEBUG;
        static const int PRIO_INFO;
        static const int PRIO_WARN;
        static const int PRIO_ERROR;
    
        static constexpr const size_t MAX_LINE = 512;
        static constexpr const size_t MAX_STORABLE_COUNT = 8196;
        static constexpr const size_t SIZE_FORMATTED_DATE = sizeof("2014-04-21 13:00:00 ") - 1;
        static constexpr const size_t SIZE_FORMATTED_PRIORITY = sizeof("[?] | ") - 1;
        static constexpr const size_t SIZE_FORMATTED_HEADER = SIZE_FORMATTED_DATE + SIZE_FORMATTED_PRIORITY;
        static constexpr const size_t SIZE_FORMATTED_CONTENT = MAX_LINE - SIZE_FORMATTED_HEADER;
    
        static inline void V(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            print(PRIO_VERBOSE, tag, format, args);
            va_end(args);
        }
        static inline void D(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            print(PRIO_DEBUG, tag, format, args);
            va_end(args);
        }
        static inline void E(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            print(PRIO_ERROR, tag, format, args);
            va_end(args);
        }
        static inline void W(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            print(PRIO_WARN, tag, format, args);
            va_end(args);
        }
        static inline void I(const char* tag, const char* format, ...) {
            va_list args;
            va_start(args, format);
            print(PRIO_INFO, tag, format, args);
            va_end(args);
        }
    
        static int32_t exportLog(char* data);
    
    private:
        static void print(int priority, const char* tag, const char* format, va_list args);
        static size_t setDate(char (&buffer)[Log::MAX_LINE]);
        static size_t setPriority(char* buffer, int priority);
        static size_t setTag(char* buffer, const char* tag);
    };
}
#endif