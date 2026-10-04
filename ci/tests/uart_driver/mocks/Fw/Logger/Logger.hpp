// Mock Fw::Logger: counts calls, prints to stderr
#ifndef FW_LOGGER_LOGGER_HPP
#define FW_LOGGER_LOGGER_HPP
#include <cstdarg>
#include <cstdio>
namespace Fw {
class Logger {
  public:
    static int s_logCount;
    static void log(const char* fmt, ...) {
        s_logCount++;
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);
    }
};
}  // namespace Fw
#endif
