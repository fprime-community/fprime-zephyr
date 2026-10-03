####
# Fw::MemAllocator implementation backed by Zephyr's shared_multi_heap pool
####
register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrSharedMultiHeapAllocator.cpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrSharedMultiHeapAllocator.hpp"
    DEPENDS
        Fw_Types
        kernel
)
