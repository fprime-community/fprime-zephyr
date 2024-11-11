add_custom_target("${FPRIME_CURRENT_MODULE}")

set(SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/error.cpp"
)
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/error.hpp"
)
set(MOD_DEPS config)
register_fprime_module(Os_Zephyr_Shared)

add_fprime_supplied_os_module(Task Zephyr kernel Os_Zephyr_Shared)
add_fprime_supplied_os_module(Console Zephyr kernel Os_Zephyr_Shared)
add_fprime_supplied_os_module("Mutex;ConditionVariable" Zephyr kernel Os_Zephyr_Shared)
add_fprime_supplied_os_module(RawTime Zephyr kernel Os_Zephyr_Shared)
add_fprime_supplied_os_module(Queue Zephyr kernel Os_Zephyr_Shared)
