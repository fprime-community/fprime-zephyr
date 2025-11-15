####
# unix/Platform/CMakeLists.txt:
#
# This file defines a configuration module for unix platforms.
####
register_fprime_config(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/PlatformTypes.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/PlatformTypes.h"
    CHOOSES_IMPLEMENTATIONS
        Os_Task_Zephyr
        Os_Generic_PriorityQueue
        Os_Mutex_Zephyr
        Os_RawTime_Zephyr
        Os_StringFormat_Zephyr_snprintk
        Os_Console_Zephyr
        Os_File_Zephyr
        # Can remain stubs for now
        Os_Cpu_Stub
        Os_Memory_Stub
    INTERFACE # No buildable files generated
)
