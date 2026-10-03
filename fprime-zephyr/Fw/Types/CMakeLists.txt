####
# Fw::MemAllocator implementations backed by Zephyr multi-heaps
####
register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrMultiHeapAllocator.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrSharedMultiHeapAllocator.cpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrMultiHeapAllocator.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/ZephyrHeapRegion.hpp"
    DEPENDS
        Fw_Types
        kernel
)
