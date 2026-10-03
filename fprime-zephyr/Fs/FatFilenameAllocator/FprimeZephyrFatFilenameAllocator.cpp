// ======================================================================
// \title  FprimeZephyrFatFilenameAllocator.cpp
// \brief  Static FatFs long-filename buffer pool installed in place of ff_memalloc/ff_memfree
// ======================================================================
#include "fprime-zephyr/Fs/FatFilenameAllocator/FprimeZephyrFatFilenameAllocator.hpp"

#include <ff.h>
#include <zephyr/spinlock.h>

#include "zephyr-config/FatFilenameAllocatorCfg.hpp"

static_assert(FF_USE_LFN == 3, "FprimeZephyrFatFilenameAllocator requires CONFIG_FS_FATFS_LFN_MODE_HEAP");
// The slot size below mirrors the private INIT_NAMBUF / MAXDIRB macros of ff.c at this FatFs revision (R0.16)
static_assert(FF_DEFINED == 80386, "FatFs revision changed: re-verify the slot size against INIT_NAMBUF in ff.c");
#if FF_FS_REENTRANT
// FatFs holds the volume mutex while it holds a slot, so FF_VOLUMES slots make exhaustion impossible
static_assert(FatFilenameAllocatorConfig::FPRIME_ZEPHYR_FAT_FILENAME_SLOTS >= FF_VOLUMES,
              "FPRIME_ZEPHYR_FAT_FILENAME_SLOTS must be at least the number of FatFs volumes (FF_VOLUMES)");
#endif

namespace Zephyr {
namespace {

//! UTF-16 long-filename buffer requested by FatFs's INIT_NAMBUF
constexpr FwSizeType LFN_BUFFER_BYTES = (static_cast<FwSizeType>(FF_MAX_LFN) + 1U) * sizeof(WCHAR);
#if FF_FS_EXFAT
//! exFAT directory-entry block appended by INIT_NAMBUF; mirrors MAXDIRB(FF_MAX_LFN) in ff.c (SZDIRE = 32)
constexpr FwSizeType EXFAT_DIR_BLOCK_BYTES = ((static_cast<FwSizeType>(FF_MAX_LFN) + 44U) / 15U) * 32U;
#else
constexpr FwSizeType EXFAT_DIR_BLOCK_BYTES = 0U;
#endif

//! Serializes pool access with a Zephyr spinlock: callers hold only a per-volume FatFs mutex, if any
struct ZephyrSpinLockPolicy {
    using Lock = struct k_spinlock;
    class Guard {
      public:
        explicit Guard(Lock& lock) : m_lock(lock), m_key(k_spin_lock(&lock)) {}
        ~Guard() { k_spin_unlock(&this->m_lock, this->m_key); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

      private:
        Lock& m_lock;
        k_spinlock_key_t m_key;
    };
};

using Pool = FatFilenameAllocator<LFN_BUFFER_BYTES + EXFAT_DIR_BLOCK_BYTES,
                                  FatFilenameAllocatorConfig::FPRIME_ZEPHYR_FAT_FILENAME_SLOTS,
                                  ZephyrSpinLockPolicy>;

//! Constant-initialized: lives in .bss and needs no startup constructor
static_assert((Pool(), true), "Pool must be constant-initializable (no startup constructor)");
Pool s_pool;

}  // namespace

FatFilenameAllocatorStats FprimeZephyrFatFilenameAllocator::getStats() {
    return s_pool.getStats();
}

}  // namespace Zephyr

// Reserved identifiers required by GNU ld --wrap=ff_memalloc / --wrap=ff_memfree
extern "C" {
void* __wrap_ff_memalloc(UINT msize);
void __wrap_ff_memfree(void* mblock);

void* __wrap_ff_memalloc(UINT msize) {
    return Zephyr::s_pool.allocate(static_cast<FwSizeType>(msize));
}

void __wrap_ff_memfree(void* mblock) {
    Zephyr::s_pool.release(mblock);
}
}
