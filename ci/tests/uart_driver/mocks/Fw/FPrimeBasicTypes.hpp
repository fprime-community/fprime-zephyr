// Mock F Prime basic types for host-side testing
#ifndef FW_FPRIME_BASIC_TYPES_HPP
#define FW_FPRIME_BASIC_TYPES_HPP
#include <cstdint>
#include <cstddef>
typedef uint8_t U8;
typedef uint32_t U32;
typedef int32_t I32;
typedef uint64_t U64;
typedef size_t FwSizeType;
typedef int32_t FwIndexType;
typedef int32_t FwTaskPriorityType;
typedef int64_t FwAssertArgType;
#define FW_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
