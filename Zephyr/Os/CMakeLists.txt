cmake_minimum_required(VERSION 3.13)

register_fprime_module(
        Os_Task_Zephyr
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/Task.cpp"
    HEADERS
      "${CMAKE_CURRENT_LIST_DIR}/Task.hpp"
    DEPENDS
        Fw_Types
)
register_fprime_implementation(Os_Task Os_Task_Zephyr "${CMAKE_CURRENT_LIST_DIR}/DefaultTask.cpp")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    set(SOURCE_FILES
        # Common implementations
        "${FPRIME_FRAMEWORK_PATH}/Os/Pthreads/BufferQueueCommon.cpp"
        # Baremetal
        "${FPRIME_FRAMEWORK_PATH}/Os/Baremetal/SystemResources.cpp"
        # Custom
        "${CMAKE_CURRENT_LIST_DIR}/LogPrintk.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Mutex.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Queue.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/IntervalTimer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Task.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/File.cpp"
    )
    register_fprime_module(fprime-zephyr_Os)
endif()

####
# If you ever run into issues, try getting inspiration here:
#    https://github.com/fprime-community/fprime-azure-threadx/blob/fprime-azure-threadx-cmjl-14-01-2022-v00/fprime-nucleo/NucleoOs/CMakeLists.txt
####
if (${CMAKE_SYSTEM_NAME} STREQUAL "Zephyr")
    target_link_libraries(Os PRIVATE fprime-zephyr_Os)
    
    # Resolves linking errors with `k_msgq_*` functions, but not sure why.
    # See: https://github.com/zephyrproject-rtos/zephyr/issues/31613
    target_link_libraries(fprime-zephyr_Os PRIVATE kernel)
endif()