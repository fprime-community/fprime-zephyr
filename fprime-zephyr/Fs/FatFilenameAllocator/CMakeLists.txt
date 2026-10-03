####
# fprime-zephyr/Fs/FatFilenameAllocator/CMakeLists.txt:
#
# Static pool for FatFs long-filename buffers, installed with ld --wrap when CONFIG_FS_FATFS_LFN_MODE_HEAP=y.
# See docs/sdd.md.
####
if (FPRIME_PLATFORM STREQUAL "Zephyr")
    option(FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR
           "Serve FatFs long-filename buffers from a static pool when CONFIG_FS_FATFS_LFN_MODE_HEAP=y" ON)
    if (NOT CONFIG_FS_FATFS_LFN_MODE_HEAP OR NOT FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR)
        return()
    endif()
    if (CONFIG_LTO)
        message(FATAL_ERROR "FprimeZephyrFatFilenameAllocator relies on -Wl,--wrap, which does not intercept calls "
                            "resolved inside LTO. Disable CONFIG_LTO or set FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR=OFF.")
    endif()
    register_fprime_module(
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/FprimeZephyrFatFilenameAllocator.cpp"
        HEADERS
            "${CMAKE_CURRENT_LIST_DIR}/FatFilenameAllocator.hpp"
            "${CMAKE_CURRENT_LIST_DIR}/FprimeZephyrFatFilenameAllocator.hpp"
        DEPENDS
            Fw_Types
            default_zephyr-config
            kernel
            ELMFAT
    )
    skip_on_sub_build()
    if (NOT TARGET app)
        message(FATAL_ERROR "FprimeZephyrFatFilenameAllocator: Zephyr 'app' target not found; "
                            "call find_package(Zephyr) before including F Prime")
    endif()
    # Only Zephyr's FatFs references the wrapped symbols, so link the module into the image explicitly
    target_link_libraries(app PRIVATE "${FPRIME_CURRENT_MODULE}")
    zephyr_ld_options(-Wl,--wrap=ff_memalloc -Wl,--wrap=ff_memfree)
elseif (BUILD_TESTING)
    find_package(Threads REQUIRED)
    register_fprime_ut(
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/test/ut/FatFilenameAllocatorTestMain.cpp"
        DEPENDS
            Fw_Types
            Threads::Threads
    )
endif()
