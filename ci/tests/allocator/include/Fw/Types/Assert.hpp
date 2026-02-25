#ifndef FW_TYPES_ASSERT_HPP
#define FW_TYPES_ASSERT_HPP

#include <cassert>

#ifndef FW_ASSERT
#define FW_ASSERT(condition, ...) assert(condition)
#endif

#endif
