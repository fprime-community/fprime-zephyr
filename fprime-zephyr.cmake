set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

# TEMPORARY FIX: Add ASSERT_RELATIVE_PATH definition for FW_ASSERT_LEVEL = FW_RELATIVE_PATH_ASSERT
if(NOT DEFINED ASSERT_RELATIVE_PATH)
    add_definitions(-DASSERT_RELATIVE_PATH="${FPRIME_PROJECT_ROOT}")
endif()

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Os")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Svc/ZephyrTime")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrRateDriver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrUartDriver")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Zephyr/Drv/ZephyrGpioDriver")
