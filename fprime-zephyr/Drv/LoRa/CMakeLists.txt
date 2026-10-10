####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/docs/reference/api/cmake/API/
#
####

# Module names are derived from the path from the nearest project/library/framework
# root when not specifically overridden by the developer. i.e. The module defined by
# `Ref/SignalGen/CMakeLists.txt` will be named `Ref_SignalGen`.

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/LoRa.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/LoRa.cpp"
   DEPENDS
       default_zephyr-config
)

# Modem status is read through the loramac-node radio HAL when a Semtech SX127x is present
if(DEFINED ZEPHYR_LORAMAC_NODE_MODULE_DIR)
    target_include_directories(fprime-zephyr_Drv_LoRa PRIVATE "${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio")
endif()

### Unit Tests ###
# register_fprime_ut(
#     AUTOCODER_INPUTS
#         "${CMAKE_CURRENT_LIST_DIR}/LoRa.fpp"
#     SOURCES
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/LoRaTestMain.cpp"
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/LoRaTester.cpp"
#     DEPENDS
#         STest # For rules-based testing
#     UT_AUTO_HELPERS
# )
