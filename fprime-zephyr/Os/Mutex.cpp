// ======================================================================
// \title Os/Zephyr/Mutex.cpp
// \brief Zephyr implementation for Os::Mutex
// Relevant Documentation:
// https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__mutex__apis.html
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include "fprime-zephyr/Os/Mutex.hpp"

namespace Os {
namespace Zephyr {
namespace Mutex {

ZephyrMutex::ZephyrMutex() : Os::MutexInterface(), m_handle() {
    // set attributes
    k_mutex_init(&this->m_handle.m_mutex_descriptor);
}

ZephyrMutex::~ZephyrMutex() {}

ZephyrMutex::Status ZephyrMutex::take() {
    int status = k_mutex_lock(&this->m_handle.m_mutex_descriptor, K_FOREVER);
    if(status == 0){
        return Os::Mutex::Status::OP_OK;
    } else if (status == -EBUSY){
        return Os::Mutex::Status::ERROR_BUSY;
    } else { // -EAGAIN : mutex timed out
        return Os::Mutex::Status::NOT_SUPPORTED; 
    }

}

ZephyrMutex::Status ZephyrMutex::release() {
    int status = k_mutex_unlock(&this->m_handle.m_mutex_descriptor);
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
