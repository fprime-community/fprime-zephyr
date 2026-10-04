// Mock FW_ASSERT: abort with location so a failing assert fails the test
#ifndef FW_TYPES_ASSERT_HPP
#define FW_TYPES_ASSERT_HPP
#include <cstdio>
#include <cstdlib>
#define FW_ASSERT(cond, ...)                                                      \
    do {                                                                          \
        if (!(cond)) {                                                            \
            std::fprintf(stderr, "FW_ASSERT failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            std::abort();                                                         \
        }                                                                         \
    } while (0)
#endif
