#include <Os/Mutex.hpp>
#include <Fw/Types/Assert.hpp>

#include <zephyr/kernel.h>

namespace Os {

    Mutex::Mutex() : m_handle(0) {
        struct k_mutex* mutex = reinterpret_cast<struct k_mutex *>(k_malloc(sizeof(struct k_mutex)));
        FW_ASSERT(mutex != nullptr);

        NATIVE_INT_TYPE ret = k_mutex_init(mutex);
        FW_ASSERT(ret == 0, ret);

        this->m_handle = reinterpret_cast<POINTER_CAST>(mutex);
    }

    Mutex::~Mutex() {
        FW_ASSERT(this->m_handle != 0);
        k_free(reinterpret_cast<struct k_mutex*>(this->m_handle));
    }

    void Mutex::lock() {
        FW_ASSERT(this->m_handle != 0);
        NATIVE_INT_TYPE ret = k_mutex_lock(reinterpret_cast<struct k_mutex*>(this->m_handle), K_FOREVER);
        FW_ASSERT(ret == 0, ret);
    }

    void Mutex::unLock() {
        FW_ASSERT(this->m_handle != 0);
        NATIVE_INT_TYPE ret = k_mutex_unlock(reinterpret_cast<struct k_mutex*>(this->m_handle));
        FW_ASSERT(ret == 0, ret);
    }

}