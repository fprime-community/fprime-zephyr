// ======================================================================
// \title Os/Zephyr/Mutex.cpp
// \brief Zephyr implementation for Os::Mutex
// ======================================================================
#include "Zephyr/Os/Mutex.hpp"
#include <Fw/Types/Assert.hpp>

namespace Os {
namespace Zephyr {

ZephyrMutex::ZephyrMutex() {
    struct k_mutex* mutex = reinterpret_cast<struct k_mutex *>(k_malloc(sizeof(struct k_mutex)));
    FW_ASSERT(mutex != nullptr);

    NATIVE_INT_TYPE ret = k_mutex_init(mutex);
    FW_ASSERT(ret == 0, ret);

    this->m_handle.m_mutex_descriptor = mutex;
}

ZephyrMutex::~ZephyrMutex() {
    FW_ASSERT(this->m_handle.m_mutex_descriptor != nullptr);
    k_free(this->m_handle.m_mutex_descriptor);
}

ZephyrMutex::Status ZephyrMutex::take() {
    FW_ASSERT(this->m_handle.m_mutex_descriptor != nullptr);
    NATIVE_INT_TYPE ret = k_mutex_lock(this->m_handle.m_mutex_descriptor, K_FOREVER);
    return (ret == 0) ? Status::OP_OK : Status::ERROR_OTHER;
}

ZephyrMutex::Status ZephyrMutex::release() {
    FW_ASSERT(this->m_handle.m_mutex_descriptor != nullptr);
    NATIVE_INT_TYPE ret = k_mutex_unlock(this->m_handle.m_mutex_descriptor);
    return (ret == 0) ? Status::OP_OK : Status::ERROR_OTHER;
}

MutexHandle* ZephyrMutex::getHandle() {
    return &this->m_handle;
}

}  // namespace Zephyr
}  // namespace Os
