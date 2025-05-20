// ======================================================================
// \title Os/Zephyr/Mutex.cpp
// \brief Zephyr implementation for Os::Mutex
// ======================================================================
#include <Fw/Types/Assert.hpp>
// #include <Os/Zephyr/error.hpp>
#include "Mutex.hpp"

namespace Os {
namespace Zephyr {
namespace Mutex {

ZephyrMutex::ZephyrMutex() : Os::MutexInterface(), m_handle() {
    // set attributes
    struct k_mutex* mutex = reinterpret_cast<struct k_mutex *>(k_malloc(sizeof(struct k_mutex)));
    k_mutex_init(mutex);
}

ZephyrMutex::~ZephyrMutex() {
    k_free(reinterpret_cast<struct k_mutex*>(this->m_handle.m_mutex_descriptor));
}

ZephyrMutex::Status ZephyrMutex::take() {
    int status = k_mutex_lock(reinterpret_cast<struct k_mutex*>(this->m_handle.m_mutex_descriptor), K_FOREVER);
    if(status == 0){
        return Os::Mutex::Status::OP_OK;
    } else if (status == -EBUSY){
        return Os::Mutex::Status::ERROR_BUSY;
    } else { // -EAGAIN : mutex timed out
        return Os::Mutex::Status::NOT_SUPPORTED; 
    }

}

ZephyrMutex::Status ZephyrMutex::release() {
    int status = k_mutex_unlock(reinterpret_cast<struct k_mutex*>(this->m_handle.m_mutex_descriptor));
    if(status == 0){
        return Os::Mutex::Status::OP_OK;
    }else { // -EPERM and -EINVAL are flags not supported by Fprime
        return Os::Mutex::Status::NOT_SUPPORTED; 
    }
}

MutexHandle* ZephyrMutex::getHandle() {
    return &this->m_handle;
}
}  // namespace Mutex
}  // namespace Zephyr
}  // namespace Os