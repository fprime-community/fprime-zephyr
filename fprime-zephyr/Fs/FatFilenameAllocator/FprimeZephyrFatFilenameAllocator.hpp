// ======================================================================
// \title  FprimeZephyrFatFilenameAllocator.hpp
// \brief  Static FatFs long-filename buffer pool installed in place of ff_memalloc/ff_memfree
// ======================================================================
#ifndef FPRIME_ZEPHYR_FS_FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR_HPP
#define FPRIME_ZEPHYR_FS_FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR_HPP

#include "fprime-zephyr/Fs/FatFilenameAllocator/FatFilenameAllocator.hpp"

namespace Zephyr {

//! \brief Counters of the pool that FatFs reaches through the linker-wrapped ff_memalloc/ff_memfree
class FprimeZephyrFatFilenameAllocator {
  public:
    FprimeZephyrFatFilenameAllocator() = delete;

    //! \brief Read a consistent snapshot of the pool counters
    static FatFilenameAllocatorStats getStats();
};

}  // namespace Zephyr

#endif  // FPRIME_ZEPHYR_FS_FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR_HPP
