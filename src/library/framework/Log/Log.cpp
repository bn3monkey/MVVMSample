#include "Log.hpp"

using namespace Bn3Monkey;

class LogStream
{
public :
    LogStream() : start_pos(0), end_pos(0) {
    }
    void read(const char(&buffer)[Log::MAX_LINE])
    {
        {
            std::lock_guard<std::mutex> lock(stream_mtx);
            char(&target)[Log::MAX_LINE] = data[end_pos];

            std::copy(buffer, buffer + Log::MAX_LINE, target);

            end_pos = (end_pos + 1) % Log::MAX_STORABLE_COUNT;
            if (start_pos == end_pos)
            {
                start_pos = (start_pos + 1) % Log::MAX_STORABLE_COUNT;
            }
        }
    }
    int32_t write(char* buffer)
    {
        int32_t written_pos = 0;
        {
            std::lock_guard<std::mutex> lock(stream_mtx);
            for (size_t writing_pos = start_pos; writing_pos != end_pos; writing_pos = (writing_pos + 1) % Log::MAX_STORABLE_COUNT)
            {
                char(&target)[Log::MAX_LINE] = data[writing_pos];
                size_t log_size = strlen(target);
                std::copy(target, target + log_size, buffer + written_pos);
                written_pos += log_size;
            }
        }
        return written_pos;
    }
private:
    std::mutex stream_mtx;
    size_t start_pos;
    size_t end_pos;
    char data[Log::MAX_STORABLE_COUNT][Log::MAX_LINE] = { {0} };
};
static LogStream logStream;

#if defined _WIN32 //WINDOWS
const int Bn3Monkey::Log::PRIO_VERBOSE = 0;
const int Bn3Monkey::Log::PRIO_DEBUG = 1;
const int Bn3Monkey::Log::PRIO_INFO = 2;
const int Bn3Monkey::Log::PRIO_WARN = 3;
const int Bn3Monkey::Log::PRIO_ERROR = 4;
#elif defined __ANDROID__ //ANDROID
const int Bn3Monkey::Log::PRIO_VERBOSE = ANDROID_LOG_VERBOSE;
const int Bn3Monkey::Log::PRIO_DEBUG = ANDROID_LOG_DEBUG;
const int Bn3Monkey::Log::PRIO_INFO = ANDROID_LOG_INFO;
const int Bn3Monkey::Log::PRIO_WARN = ANDROID_LOG_WARN;
const int Bn3Monkey::Log::PRIO_ERROR = ANDROID_LOG_ERROR;
#elif (defined __linux__) && !(defined __ANDROID__) // LINUX
const int Bn3Monkey::Log::PRIO_VERBOSE = 0;
const int Bn3Monkey::Log::PRIO_DEBUG = 1;
const int Bn3Monkey::Log::PRIO_INFO = 2;
const int Bn3Monkey::Log::PRIO_WARN = 3;
const int Bn3Monkey::Log::PRIO_ERROR = 4;
#endif

std::mutex console_mtx;

void Bn3Monkey::Log::print(int priority, const char* tag, const char* format, va_list args)
{
    char formatted[Log::SIZE_FORMATTED_CONTENT] = { 0 };
    size_t formatted_size =
#if defined _WIN32
        vsprintf_s(formatted, format, args);
#elif defined __ANDROID__
        vsprintf(formatted, format, args);
#elif (defined __linux__) && !(defined __ANDROID__)
        vsprintf(formatted, format, args);
#endif

#if defined __ANDROID__
        __android_log_vprint(priority, tag, format, args);
#endif
    formatted[formatted_size] = '\n';
    formatted[formatted_size + 1] = '\0';
    formatted_size += 1;

    char buffer[Log::MAX_LINE] = { 0 };
    size_t written_size = setDate(buffer);
    written_size += setPriority(buffer + written_size, priority);
    written_size += setTag(buffer + written_size, tag);
    std::copy(formatted, formatted + formatted_size, buffer + written_size);
    

#if defined _WIN32
    OutputDebugStringA(buffer);
#endif
    {
        std::lock_guard<std::mutex> lock(console_mtx);
        printf(buffer);
    }

    logStream.read(buffer);
}

int32_t Bn3Monkey::Log::exportLog(char* data)
{
    int32_t ret = logStream.write(data);
    return ret;
}

size_t Bn3Monkey::Log::setDate(char(&buffer)[Log::MAX_LINE])
{
    time_t now = time(0);
    struct tm tstruct;
#if defined _WIN32
    localtime_s(&tstruct, &now);
#else
    localtime_r(&now, &tstruct);
#endif

    size_t written_size =
#if defined _WIN32
    snprintf(buffer, SIZE_FORMATTED_DATE + 1, "%04d-%02d-%02d %02d:%02d:%02d ", tstruct.tm_year + 1900, tstruct.tm_mon, tstruct.tm_mday, tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec);
#else
    snprintf(buffer, SIZE_FORMATTED_DATE + 1, "%d-%d-%d %02d:%02d:%02d ", tstruct.tm_year + 1900, tstruct.tm_mon, tstruct.tm_mday, tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec);
#endif
    return SIZE_FORMATTED_DATE;
}

size_t Bn3Monkey::Log::setPriority(char* buffer, int priority)
{
    const char* priority_str = "[?] | ";
    switch (priority)
    {
    case Log::PRIO_VERBOSE:
        priority_str = "[V] | ";
        break;
    case Log::PRIO_DEBUG:
        priority_str = "[D] | ";
        break;
    case Log::PRIO_INFO:
        priority_str = "[I] | ";
        break;
    case Log::PRIO_WARN:
        priority_str = "[W] | ";
        break;
    case Log::PRIO_ERROR:
        priority_str = "[E] | ";
        break;
    }
    size_t size = sizeof("[?] | ");
    std::copy(priority_str, priority_str + size, buffer);
    return SIZE_FORMATTED_PRIORITY;
}

size_t Bn3Monkey::Log::setTag(char* buffer, const char* tag)
{
    buffer[0] = '{';
    size_t tag_size = strlen(tag);
    std::copy(tag, tag + tag_size, buffer + 1);
    buffer[tag_size + 1] = '}';
    buffer[tag_size + 2] = ' ';
    buffer[tag_size + 3] = '\0';
    return tag_size + 3;
}
