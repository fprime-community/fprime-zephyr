// ======================================================================
// \title  FatFilenameAllocatorTestMain.cpp
// \brief  Host unit tests for Zephyr::FatFilenameAllocator
// ======================================================================
#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "fprime-zephyr/Fs/FatFilenameAllocator/FatFilenameAllocator.hpp"

namespace {

//! Host stand-in for the Zephyr spinlock policy
struct HostMutexPolicy {
    using Lock = std::mutex;
    using Guard = std::lock_guard<std::mutex>;
};

// exFAT-enabled FatFs with FF_MAX_LFN = 255 requests (255 + 1) * 2 + (255 + 44) / 15 * 32 = 1120 bytes
constexpr FwSizeType TEST_SLOT_BYTES = 1120U;
constexpr FwSizeType TEST_SLOT_COUNT = 4U;
using TestPool = Zephyr::FatFilenameAllocator<TEST_SLOT_BYTES, TEST_SLOT_COUNT, HostMutexPolicy>;

//! Literal lock policy: lets the pool be constructed in a constant expression below
struct NoLockPolicy {
    struct Lock {};
    struct Guard {
        explicit constexpr Guard(Lock&) {}
    };
};
static_assert((Zephyr::FatFilenameAllocator<TEST_SLOT_BYTES, TEST_SLOT_COUNT, NoLockPolicy>(), true),
              "Pool default construction must be a constant initialization (FZFA-007)");

//! Records asserts instead of aborting so that the asserting path and pool state can be checked
class RecordingAssertHook : public Fw::AssertHook {
  public:
    void reportAssert(FILE_NAME_ARG file,
                      FwSizeType lineNo,
                      FwSizeType numArgs,
                      FwAssertArgType arg1,
                      FwAssertArgType arg2,
                      FwAssertArgType arg3,
                      FwAssertArgType arg4,
                      FwAssertArgType arg5,
                      FwAssertArgType arg6) override {
        (void)file;
        (void)numArgs;
        (void)arg2;
        (void)arg3;
        (void)arg4;
        (void)arg5;
        (void)arg6;
        if (this->m_count == 0U) {
            this->m_firstLine = lineNo;
            this->m_firstArg = arg1;
        }
        this->m_count++;
    }
    void doAssert() override {}

    FwSizeType m_count = 0U;
    FwSizeType m_firstLine = 0U;
    FwAssertArgType m_firstArg = 0;
};

class FatFilenameAllocatorTest : public ::testing::Test {
  protected:
    void fill(TestPool& pool, void* (&slots)[TEST_SLOT_COUNT]) {
        for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
            slots[i] = pool.allocate(TEST_SLOT_BYTES);
            ASSERT_NE(slots[i], nullptr) << "slot " << i;
        }
    }
    //! Release ptr with a recording assert hook installed; checks the first assert argument and resulting in-use count
    //! \return number of asserts raised
    FwSizeType expectReleaseAssert(void* const ptr,
                                   const FwAssertArgType expectedFirstArg,
                                   const FwSizeType expectedInUse) {
        RecordingAssertHook hook;
        hook.registerHook();
        this->m_pool.release(ptr);
        hook.deregisterHook();
        EXPECT_GE(hook.m_count, 1U);
        EXPECT_EQ(hook.m_firstArg, expectedFirstArg);
        EXPECT_EQ(this->m_pool.getStats().inUse, expectedInUse);
        return hook.m_count;
    }
    TestPool m_pool;
};

TEST_F(FatFilenameAllocatorTest, InitialStats) {
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.slotCount, TEST_SLOT_COUNT);
    EXPECT_EQ(stats.slotBytes, TEST_SLOT_BYTES);
    EXPECT_EQ(stats.inUse, 0U);
    EXPECT_EQ(stats.highWaterMark, 0U);
    EXPECT_EQ(stats.exhaustedCount, 0U);
    EXPECT_EQ(stats.rejectedCount, 0U);
}

TEST_F(FatFilenameAllocatorTest, AllocateAllSlotsThenExhaust) {
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
    for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
        for (FwSizeType j = i + 1U; j < TEST_SLOT_COUNT; j++) {
            EXPECT_NE(slots[i], slots[j]);
        }
    }
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), nullptr);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), nullptr);
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.inUse, TEST_SLOT_COUNT);
    EXPECT_EQ(stats.highWaterMark, TEST_SLOT_COUNT);
    EXPECT_EQ(stats.exhaustedCount, 2U);
    EXPECT_EQ(stats.rejectedCount, 0U);
}

TEST_F(FatFilenameAllocatorTest, SlotsAreAlignedAndDisjoint) {
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
    for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(slots[i]) % alignof(std::max_align_t), 0U);
        std::memset(slots[i], static_cast<int>(0xA0U + i), TEST_SLOT_BYTES);
    }
    for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
        const U8* bytes = static_cast<const U8*>(slots[i]);
        for (FwSizeType b = 0; b < TEST_SLOT_BYTES; b++) {
            ASSERT_EQ(bytes[b], static_cast<U8>(0xA0U + i)) << "slot " << i << " byte " << b;
        }
    }
}

TEST_F(FatFilenameAllocatorTest, ReleaseAndReuseFirstFree) {
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
    this->m_pool.release(slots[2]);
    this->m_pool.release(slots[1]);
    EXPECT_EQ(this->m_pool.getStats().inUse, TEST_SLOT_COUNT - 2U);
    // First-free scan hands back the lowest free slot
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), slots[1]);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), slots[2]);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), nullptr);
    for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
        this->m_pool.release(slots[i]);
    }
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.inUse, 0U);
    EXPECT_EQ(stats.highWaterMark, TEST_SLOT_COUNT);
    EXPECT_EQ(stats.exhaustedCount, 1U);
}

TEST_F(FatFilenameAllocatorTest, RepeatedCyclesDoNotLeak) {
    for (FwSizeType cycle = 0; cycle < 100U; cycle++) {
        void* slots[TEST_SLOT_COUNT] = {};
        this->fill(this->m_pool, slots);
        for (FwSizeType i = 0; i < TEST_SLOT_COUNT; i++) {
            this->m_pool.release(slots[i]);
        }
    }
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.inUse, 0U);
    EXPECT_EQ(stats.exhaustedCount, 0U);
}

TEST_F(FatFilenameAllocatorTest, WrongSizeRequestsRejected) {
    EXPECT_EQ(this->m_pool.allocate(0U), nullptr);
    EXPECT_EQ(this->m_pool.allocate(1U), nullptr);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES - 1U), nullptr);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES + 1U), nullptr);
    EXPECT_EQ(this->m_pool.allocate(TestPool::SLOT_STRIDE + 1U), nullptr);
    // Sizes FatFs dir_clear() tries: 32 KiB halving down to one 512 B sector
    for (FwSizeType size = 0x8000U; size >= 512U; size /= 2U) {
        EXPECT_EQ(this->m_pool.allocate(size), nullptr) << "size " << size;
    }
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.rejectedCount, 12U);
    EXPECT_EQ(stats.exhaustedCount, 0U);
    EXPECT_EQ(stats.inUse, 0U);
    EXPECT_EQ(stats.highWaterMark, 0U);
    // Rejections consume no slots
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
}

TEST_F(FatFilenameAllocatorTest, WrongSizeRejectedWhenExhaustedCountsAsRejected) {
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES + 1U), nullptr);
    const Zephyr::FatFilenameAllocatorStats stats = this->m_pool.getStats();
    EXPECT_EQ(stats.rejectedCount, 1U);
    EXPECT_EQ(stats.exhaustedCount, 0U);
}

TEST_F(FatFilenameAllocatorTest, ReleaseNullIsNoOp) {
    RecordingAssertHook hook;
    hook.registerHook();
    this->m_pool.release(nullptr);
    hook.deregisterHook();
    EXPECT_EQ(hook.m_count, 0U);
    EXPECT_EQ(this->m_pool.getStats().inUse, 0U);
}

TEST_F(FatFilenameAllocatorTest, ForeignPointersAssert) {
    void* slot = this->m_pool.allocate(TEST_SLOT_BYTES);
    ASSERT_NE(slot, nullptr);
    U8 stackBuffer[TEST_SLOT_BYTES] = {};
    TestPool otherPool;
    void* otherSlot = otherPool.allocate(TEST_SLOT_BYTES);
    ASSERT_NE(otherSlot, nullptr);
    void* const foreign[] = {
        stackBuffer, otherSlot,
        static_cast<U8*>(slot) + 1,                    // inside a slot, not its start
        static_cast<U8*>(slot) + TEST_SLOT_BYTES - 1,  // last byte of a slot
    };
    for (void* const ptr : foreign) {
        // The slot-membership check fires first and reports the "not found" index
        this->expectReleaseAssert(ptr, static_cast<FwAssertArgType>(TEST_SLOT_COUNT), 1U);
    }
    // The legitimate slot is unaffected and still releasable
    RecordingAssertHook hook;
    hook.registerHook();
    this->m_pool.release(slot);
    hook.deregisterHook();
    EXPECT_EQ(hook.m_count, 0U);
    EXPECT_EQ(this->m_pool.getStats().inUse, 0U);
    otherPool.release(otherSlot);
}

TEST_F(FatFilenameAllocatorTest, DoubleFreeAsserts) {
    void* slots[TEST_SLOT_COUNT] = {};
    this->fill(this->m_pool, slots);
    this->m_pool.release(slots[3]);
    EXPECT_EQ(this->expectReleaseAssert(slots[3], static_cast<FwAssertArgType>(3), TEST_SLOT_COUNT - 1U), 1U);
    // State is not corrupted: exactly one slot is available again
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), slots[3]);
    EXPECT_EQ(this->m_pool.allocate(TEST_SLOT_BYTES), nullptr);
}

TEST_F(FatFilenameAllocatorTest, ReleaseOfNeverAllocatedSlotAsserts) {
    void* slot0 = this->m_pool.allocate(TEST_SLOT_BYTES);
    ASSERT_NE(slot0, nullptr);
    void* slot1 = static_cast<U8*>(slot0) + TestPool::SLOT_STRIDE;  // start of slot 1, never handed out
    EXPECT_EQ(this->expectReleaseAssert(slot1, static_cast<FwAssertArgType>(1), 1U), 1U);
}

using FatFilenameAllocatorDeathTest = FatFilenameAllocatorTest;

TEST_F(FatFilenameAllocatorDeathTest, DefaultAssertHookAbortsOnForeignPointer) {
    U8 stackBuffer[16] = {};
    EXPECT_DEATH(this->m_pool.release(stackBuffer), "Assert");
}

TEST_F(FatFilenameAllocatorDeathTest, DefaultAssertHookAbortsOnDoubleFree) {
    void* slot = this->m_pool.allocate(TEST_SLOT_BYTES);
    ASSERT_NE(slot, nullptr);
    this->m_pool.release(slot);
    EXPECT_DEATH(this->m_pool.release(slot), "Assert");
}

TEST(FatFilenameAllocatorThreadTest, ConcurrentAllocateReleaseNeverSharesASlot) {
    constexpr FwSizeType THREADS = 8U;
    constexpr FwSizeType ITERATIONS = 20000U;
    static TestPool pool;
    std::atomic<FwSizeType> successes(0U);
    std::atomic<FwSizeType> failures(0U);
    std::atomic<FwSizeType> corruptions(0U);
    std::atomic<FwSizeType> holders(0U);
    std::atomic<FwSizeType> maxHolders(0U);
    std::vector<std::thread> threads;
    for (FwSizeType t = 0; t < THREADS; t++) {
        threads.emplace_back([t, &successes, &failures, &corruptions, &holders, &maxHolders]() {
            const U8 pattern = static_cast<U8>(t + 1U);
            for (FwSizeType i = 0; i < ITERATIONS; i++) {
                void* slot = pool.allocate(TEST_SLOT_BYTES);
                if (slot == nullptr) {
                    failures++;
                    std::this_thread::yield();
                    continue;
                }
                const FwSizeType now = ++holders;
                FwSizeType seen = maxHolders.load();
                while (now > seen && !maxHolders.compare_exchange_weak(seen, now)) {
                }
                U8* bytes = static_cast<U8*>(slot);
                std::memset(bytes, pattern, TEST_SLOT_BYTES);
                std::this_thread::yield();
                for (FwSizeType b = 0; b < TEST_SLOT_BYTES; b++) {
                    if (bytes[b] != pattern) {
                        corruptions++;
                        break;
                    }
                }
                holders--;
                pool.release(slot);
                successes++;
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    const Zephyr::FatFilenameAllocatorStats stats = pool.getStats();
    EXPECT_EQ(corruptions.load(), 0U);
    EXPECT_EQ(successes.load() + failures.load(), THREADS * ITERATIONS);
    EXPECT_GT(successes.load(), 0U);
    EXPECT_LE(maxHolders.load(), TEST_SLOT_COUNT);
    EXPECT_EQ(stats.inUse, 0U);
    EXPECT_LE(stats.highWaterMark, TEST_SLOT_COUNT);
    EXPECT_EQ(stats.exhaustedCount, failures.load());
    EXPECT_EQ(stats.rejectedCount, 0U);
}

}  // namespace

int main(int argc, char** argv) {
    // gtest's default clone()-based death-test child crashes under AddressSanitizer; force fork()
    char useFork[] = "--gtest_death_test_use_fork";
    std::vector<char*> args(argv, argv + argc);
    args.push_back(useFork);
    int count = static_cast<int>(args.size());
    args.push_back(nullptr);
    ::testing::InitGoogleTest(&count, args.data());
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    return RUN_ALL_TESTS();
}
