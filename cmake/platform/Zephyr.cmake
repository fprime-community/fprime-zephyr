####
# Zephyr.cmake:
#
# Zephyr platform file for standard Zephyr targets.
####
link_libraries(zephyr_interface)

# Use common linux setup
set(FPRIME_HAS_SOCKETS OFF)

# Add unix specific configuration into the system
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/zephyr/Platform/")
