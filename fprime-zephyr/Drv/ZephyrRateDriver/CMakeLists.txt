####
# F prime CMakeLists.txt
####

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ZephyrRateDriver.fpp"
  SOURCES
   "${CMAKE_CURRENT_LIST_DIR}/ZephyrRateDriver.cpp"
  DEPENDS
    kernel
)
