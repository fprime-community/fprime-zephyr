set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Os")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/ZephyrTime")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrRateDriver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrUartDriver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrGpioDriver")
