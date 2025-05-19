####
# Zephyr.cmake:
#
# Zephyr platform file for standard Zephyr targets.
####
link_libraries(zephyr_interface)
# Required for Fprime
choose_fprime_implementation(Os/Task Os/Task/Zephyr)
choose_fprime_implementation(Os/Queue Os/Generic/PriorityQueue)
choose_fprime_implementation(Os/Mutex Os/Mutex/Stub)
choose_fprime_implementation(Os/RawTime Os/RawTime/Stub)

# Can remain stubs for now
choose_fprime_implementation(Os/File Os/File/Stub)
choose_fprime_implementation(Os/Console Os/Console/Stub)
choose_fprime_implementation(Os/Cpu Os/Cpu/Stub)
choose_fprime_implementation(Os/Memory Os/Memory/Stub)

# Use common linux setup
set(FPRIME_USE_POSIX OFF)
set(FPRIME_HAS_SOCKETS OFF)

# Add unix specific configuration into the system
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/zephyr/Platform/")
