####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding diles
# MOD_DEPS: (optional) module dependencies
#
####
set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/ZephyrRateDriver.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/ZephyrRateDriver.cpp"
)

register_fprime_module()
