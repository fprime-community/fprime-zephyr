#ifndef OS_STD_ATOMIC
#define OS_STD_ATOMIC

#include <zephyr/spinlock.h>  // for k_spinlock

// The zephyr/spinlock.h include will also include utility headers that define a
// a macro called EMPTY. This conflicts with the F' Queue::Status::EMPTY.
// So we need to undefine it.
#ifdef EMPTY
#undef EMPTY
#endif

namespace std {

// Memory order enum to match std::memory_order
enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
};

/**
 * Custom implementation of std::atomic that mimics the standard library interface
 * but uses Zephyr's k_spinlock
 * This is safe for ISRs
 */
template<typename T>
class atomic {
private:
    volatile T m_value;
    mutable struct k_spinlock m_lock;

public:
    atomic() noexcept : m_value() {}

    constexpr atomic(T desired) noexcept : m_value(desired) {}

    atomic(const atomic&) = delete;

    atomic& operator=(const atomic&) = delete;

    T operator=(T desired) noexcept {
        store(desired);
        return desired;
    }

    T load(memory_order order = memory_order_seq_cst) const noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        T val = m_value;
        k_spin_unlock(&m_lock, key);
        return val;
    }

    void store(T desired, memory_order order = memory_order_seq_cst) noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        m_value = desired;
        k_spin_unlock(&m_lock, key);
    }

    T exchange(T desired, memory_order order = memory_order_seq_cst) noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        T old = m_value;
        m_value = desired;
        k_spin_unlock(&m_lock, key);
        return old;
    }

    bool compare_exchange_weak(T& expected, T desired,
                               memory_order success = memory_order_seq_cst,
                               memory_order failure = memory_order_seq_cst) noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        if (m_value == expected) {
            m_value = desired;
            k_spin_unlock(&m_lock, key);
            return true;
        } else {
            expected = m_value;
            k_spin_unlock(&m_lock, key);
            return false;
        }
    }

    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order success = memory_order_seq_cst,
                                 memory_order failure = memory_order_seq_cst) noexcept {
        return compare_exchange_weak(expected, desired, success, failure);
    }

    operator T() const noexcept {
        return load();
    }

    T fetch_add(T arg, memory_order order = memory_order_seq_cst) noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        T old = m_value;
        m_value += arg;
        k_spin_unlock(&m_lock, key);
        return old;
    }

    T fetch_sub(T arg, memory_order order = memory_order_seq_cst) noexcept {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        T old = m_value;
        m_value -= arg;
        k_spin_unlock(&m_lock, key);
        return old;
    }

    T operator++() noexcept {
        return fetch_add(1) + 1;
    }

    T operator++(int) noexcept {
        return fetch_add(1);
    }

    T operator--() noexcept {
        return fetch_sub(1) - 1;
    }

    T operator--(int) noexcept {
        return fetch_sub(1);
    }
};

}

#endif
