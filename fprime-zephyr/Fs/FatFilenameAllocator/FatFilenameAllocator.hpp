// ======================================================================
// \title  FatFilenameAllocator.hpp
// \brief  Fixed pool of equally sized buffers backing FatFs long-filename working buffers
// ======================================================================
#ifndef FPRIME_ZEPHYR_FS_FAT_FILENAME_ALLOCATOR_HPP
#define FPRIME_ZEPHYR_FS_FAT_FILENAME_ALLOCATOR_HPP

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <cstddef>

namespace Zephyr {

//! \brief Snapshot of the pool counters
struct FatFilenameAllocatorStats {
    FwSizeType slotCount = 0U;       //!< Number of slots in the pool
    FwSizeType slotBytes = 0U;       //!< Exact request size served by each slot
    FwSizeType inUse = 0U;           //!< Slots currently allocated
    FwSizeType highWaterMark = 0U;   //!< Largest value inUse has reached
    FwSizeType exhaustedCount = 0U;  //!< Correctly sized requests refused because every slot was in use
    FwSizeType rejectedCount = 0U;   //!< Requests refused because their size was not slotBytes
};

//! \brief Fixed pool of SLOT_COUNT buffers of SLOT_BYTES each, held in the object itself
//!
//! \tparam SLOT_BYTES exact request size served by each slot
//! \tparam SLOT_COUNT number of slots
//! \tparam LockPolicy type providing `Lock` (default-constructible lock object) and `Guard` (RAII guard
//!         constructed from `Lock&`) used to serialize access to the pool
template <FwSizeType SLOT_BYTES, FwSizeType SLOT_COUNT, typename LockPolicy>
class FatFilenameAllocator {
    static_assert(SLOT_BYTES > 0U, "Slot size must be non-zero");
    static_assert(SLOT_COUNT > 0U, "Slot count must be non-zero");

  public:
    //! Alignment of every slot: suitable for any fundamental type
    static constexpr FwSizeType SLOT_ALIGNMENT = alignof(std::max_align_t);
    //! Distance between consecutive slots: SLOT_BYTES rounded up to SLOT_ALIGNMENT
    static constexpr FwSizeType SLOT_STRIDE = ((SLOT_BYTES + SLOT_ALIGNMENT - 1U) / SLOT_ALIGNMENT) * SLOT_ALIGNMENT;

    constexpr FatFilenameAllocator() = default;
    FatFilenameAllocator(const FatFilenameAllocator&) = delete;
    FatFilenameAllocator& operator=(const FatFilenameAllocator&) = delete;

    //! \brief Allocate one slot
    //! \param size requested size in bytes; must equal SLOT_BYTES
    //! \return start of a free slot, or nullptr if size is not SLOT_BYTES or every slot is in use
    void* allocate(const FwSizeType size) {
        void* result = nullptr;
        typename LockPolicy::Guard guard(this->m_lock);
        if (size != SLOT_BYTES) {
            this->m_rejectedCount++;
        } else {
            for (FwSizeType i = 0; i < SLOT_COUNT; i++) {
                if (!this->m_inUse[i]) {
                    this->m_inUse[i] = true;
                    this->m_inUseCount++;
                    if (this->m_inUseCount > this->m_highWaterMark) {
                        this->m_highWaterMark = this->m_inUseCount;
                    }
                    result = &this->m_slots[i][0];
                    break;
                }
            }
            if (result == nullptr) {
                this->m_exhaustedCount++;
            }
        }
        return result;
    }

    //! \brief Return a slot to the pool; asserts if ptr is not an allocated slot start
    //!
    //! \param ptr pointer previously returned by allocate(), or nullptr
    void release(void* const ptr) {
        if (ptr == nullptr) {
            return;
        }
        FwSizeType index = SLOT_COUNT;
        bool wasInUse = false;
        {
            typename LockPolicy::Guard guard(this->m_lock);
            for (FwSizeType i = 0; i < SLOT_COUNT; i++) {
                if (ptr == static_cast<void*>(&this->m_slots[i][0])) {
                    index = i;
                    break;
                }
            }
            if (index < SLOT_COUNT) {
                wasInUse = this->m_inUse[index];
                if (wasInUse) {
                    this->m_inUse[index] = false;
                    this->m_inUseCount--;
                }
            }
        }
        // Assert outside the guard so the assert hook never runs under the lock
        FW_ASSERT(index < SLOT_COUNT, static_cast<FwAssertArgType>(index));
        FW_ASSERT(wasInUse, static_cast<FwAssertArgType>(index));
    }

    //! \brief Read a consistent snapshot of the pool counters
    FatFilenameAllocatorStats getStats() const {
        typename LockPolicy::Guard guard(this->m_lock);
        FatFilenameAllocatorStats stats;
        stats.slotCount = SLOT_COUNT;
        stats.slotBytes = SLOT_BYTES;
        stats.inUse = this->m_inUseCount;
        stats.highWaterMark = this->m_highWaterMark;
        stats.exhaustedCount = this->m_exhaustedCount;
        stats.rejectedCount = this->m_rejectedCount;
        return stats;
    }

  private:
    alignas(SLOT_ALIGNMENT) U8 m_slots[SLOT_COUNT][SLOT_STRIDE] = {};  //!< Slot storage
    bool m_inUse[SLOT_COUNT] = {};                                     //!< Allocation state of each slot
    FwSizeType m_inUseCount = 0U;                                      //!< Slots currently allocated
    FwSizeType m_highWaterMark = 0U;                                   //!< Largest m_inUseCount observed
    FwSizeType m_exhaustedCount = 0U;                                  //!< Requests refused: pool exhausted
    FwSizeType m_rejectedCount = 0U;                                   //!< Requests refused: wrong size
    mutable typename LockPolicy::Lock m_lock = {};                     //!< Serializes access to all members
};

}  // namespace Zephyr

#endif  // FPRIME_ZEPHYR_FS_FAT_FILENAME_ALLOCATOR_HPP
