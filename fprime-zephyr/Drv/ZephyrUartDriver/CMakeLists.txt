####
# F Prime CMakeLists.txt:
#
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# SOURCES: list of source files (to be compiled)
# HEADERS: list of header files
# DEPENDS: list of libraries that this module depends on
####
register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrUartDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrUartDriver.cpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrUartDriver.hpp"
    DEPENDS
        Os
        default_zephyr-config
)
