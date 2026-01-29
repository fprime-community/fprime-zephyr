cmake_minimum_required(VERSION 3.13)

register_fprime_implementation(
        Os_Task_Zephyr
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/Task.cpp"
       "${CMAKE_CURRENT_LIST_DIR}/DefaultTask.cpp"
    HEADERS
       "${CMAKE_CURRENT_LIST_DIR}/Task.hpp"
    DEPENDS
        Fw_Time
        Fw_Types
        kernel
        Fw_Time
    IMPLEMENTS
        Os_Task
)
register_fprime_implementation(
        Os_Mutex_Zephyr
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/Mutex.cpp"
       "${CMAKE_CURRENT_LIST_DIR}/ConditionVariable.cpp"
       "${CMAKE_CURRENT_LIST_DIR}/DefaultMutex.cpp"
    HEADERS
      "${CMAKE_CURRENT_LIST_DIR}/Mutex.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/ConditionVariable.hpp"
    DEPENDS
        Fw_Types
        kernel
        Fw_Time
    IMPLEMENTS
        Os_Mutex
)

register_fprime_implementation(
        Os_RawTime_Zephyr
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/RawTime.cpp"
       "${CMAKE_CURRENT_LIST_DIR}/DefaultRawTime.cpp"
    HEADERS
       "${CMAKE_CURRENT_LIST_DIR}/RawTime.hpp"
    DEPENDS
        Fw_Types
        Fw_Time
        kernel
        Fw_Time
    IMPLEMENTS
        Os_RawTime
)

register_fprime_implementation(
        Os_Console_Zephyr
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/Console.cpp"
       "${CMAKE_CURRENT_LIST_DIR}/DefaultConsole.cpp"
    HEADERS
       "${CMAKE_CURRENT_LIST_DIR}/Console.hpp"
    DEPENDS
        Fw_Types
        kernel
    IMPLEMENTS
        Os_Console
)

register_fprime_implementation(
    Os_StringFormat_Zephyr_snprintk
  IMPLEMENTS
    Fw_StringFormat
  DEPENDS
    Fw_Types
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/snprintk.cpp"
)

# Create a new File implementation that uses the File.cpp from the fprime posix implementation but provides alternate
# Directory and FileSystem implementations to work with Zephyr's subset of POSIX.
#
# Note: this implementation uses the same class names as Os/Posix/File
get_target_property(FPRIME_OS_SOURCE_DIR Os SOURCE_DIR)

# First recreate the error module from Posix since it is needed by File. Do this only if FPRIME_USE_POSIX is off
if (NOT FPRIME_USE_POSIX AND NOT TARGET Os_Posix_Shared)
    register_fprime_module(
        Os_Posix_Shared
    SOURCES
        "${FPRIME_OS_SOURCE_DIR}/Posix/error.cpp"
    HEADERS
        "${FPRIME_OS_SOURCE_DIR}/Posix/error.hpp"
    DEPENDS
        Fw_Time
        Fw_Types
    )
endif()

# Second create the File implementation
register_fprime_implementation(
    Os_File_Zephyr
  IMPLEMENTS
    Os_File
  DEPENDS
    Fw_Types
    Fw_Time
    kernel
    Os_Posix_Shared
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/File.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/FileSystem.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Directory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/DefaultFile.cpp"
)
