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

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrTouchReset.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/BootloaderEntry.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrTouchReset.cpp"
    DEPENDS
        kernel
)

### Unit Tests ###
# No unit tests yet: see docs/sdd.md "Verification"
# register_fprime_ut(
#     AUTOCODER_INPUTS
#         "${CMAKE_CURRENT_LIST_DIR}/ZephyrTouchReset.fpp"
#     SOURCES
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/ZephyrTouchResetTestMain.cpp"
#         "${CMAKE_CURRENT_LIST_DIR}/test/ut/ZephyrTouchResetTester.cpp"
#     DEPENDS
#         STest
#     UT_AUTO_HELPERS
# )
