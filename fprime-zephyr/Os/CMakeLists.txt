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

# Rewrite Os_File_Posix source with FileSystem.cpp
if (FPRIME_USE_POSIX)
    
    get_target_property(NEW_SOURCE_FILES Os_File_Posix_Implementation SOURCES)
    set(POSIX_FILE_CPP "${NEW_SOURCE_FILES}")
    list(FILTER NEW_SOURCE_FILES EXCLUDE REGEX ".*FileSystem.cpp$")
    list(FILTER NEW_SOURCE_FILES EXCLUDE REGEX ".*Directory.cpp$")
    list(FILTER POSIX_FILE_CPP INCLUDE REGEX ".*/File.cpp$")
    list(APPEND NEW_SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/FileSystem.cpp")
    list(APPEND NEW_SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/Directory.cpp")

    set_target_properties(Os_File_Posix_Implementation PROPERTIES SOURCES "${NEW_SOURCE_FILES}")
    target_compile_definitions(Os_File_Posix_Implementation PRIVATE FPRIME_SYNTHETIC_FALLOCATE=1)
endif()