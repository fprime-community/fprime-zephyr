# add_definitions(-DTGT_OS_TYPE_ZEPHYR)

# set(FPRIME_USE_BAREMETAL_SCHEDULER OFF)

# Add FPrime OSAL Implementations
choose_fprime_implementation(Os/File Os_File_Stub)

# Add Baremetal OSAL Implementations
choose_fprime_implementation(Os/Cpu Os_Cpu_Baremetal)
choose_fprime_implementation(Os/Memory Os_Memory_Baremetal)

# Add Zephyr OSAL Implementations
choose_fprime_implementation(Os/Queue Os_Queue_Zephyr)
choose_fprime_implementation(Os/Mutex Os_Mutex_Zephyr)
choose_fprime_implementation(Os/Task Os_Task_Zephyr)
choose_fprime_implementation(Os/Console Os_Console_Zephyr)
choose_fprime_implementation(Os/RawTime Os_RawTime_Zephyr)

# Zephyr compiler options
include_directories(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
)

include_directories(SYSTEM
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
)

add_compile_definitions(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
)

add_compile_options(
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    -fno-builtin
    -Wno-shadow -Wno-cast-align
    # -nostdinc
    # -ffreestanding
    # -lstdc++
    # -fno-use-cxa-atexit
    # -fno-strict-overflow
)

include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/types" "${CMAKE_CURRENT_LIST_DIR}")
