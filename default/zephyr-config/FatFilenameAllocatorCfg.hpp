// ======================================================================
// \title  zephyr-config/FatFilenameAllocatorCfg.hpp
// \brief  Configuration of the static FatFs long-filename buffer pool
// ======================================================================
#ifndef ZEPHYR_CONFIG_FAT_FILENAME_ALLOCATOR_CFG_HPP
#define ZEPHYR_CONFIG_FAT_FILENAME_ALLOCATOR_CFG_HPP
#include <Fw/FPrimeBasicTypes.hpp>
namespace FatFilenameAllocatorConfig {
//! Number of FatFs long-filename buffers: one per thread that may be inside a FatFs path call at the same time.
//! Not related to CONFIG_FS_FATFS_NUM_FILES. See fprime-zephyr/Fs/FatFilenameAllocator/docs/sdd.md.
constexpr FwSizeType FPRIME_ZEPHYR_FAT_FILENAME_SLOTS = 4U;
}  // namespace FatFilenameAllocatorConfig
#endif  // ZEPHYR_CONFIG_FAT_FILENAME_ALLOCATOR_CFG_HPP
