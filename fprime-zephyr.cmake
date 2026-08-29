# fprime-zephyr registers Zephyr-only Os implementations and drivers; guard on
# the platform so projects can also run native (host) builds — e.g. for F Prime
# unit tests — with this library still listed in library_locations.
if(FPRIME_PLATFORM STREQUAL "Zephyr")
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/default/zephyr-config")
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime-zephyr/Os")
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime-zephyr/Drv")
    add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime-zephyr/Svc")
endif()
