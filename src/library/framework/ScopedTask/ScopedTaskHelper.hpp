#ifndef __SCOPED_THREAD_POOL_HELPER__
#define __SCOPED_THREAD_POOL_HELPER__

#ifdef __linux__
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#endif

inline void setThreadName(const char* name)
{
#ifdef __linux__
    pthread_setname_np(pthread_self(), name);
#elif _WIN32
    wchar_t buffer[256] = { 0 };
    int size = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    MultiByteToWideChar(CP_UTF8, 0, name, -1, buffer, size);
    SetThreadDescription(GetCurrentThread(), buffer);
#endif
}

#endif