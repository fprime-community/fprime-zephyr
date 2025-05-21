#include "Zephyr/Os/ConditionVariable.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Logger/Logger.hpp"
#include "Zephyr/Os/Mutex.hpp"

//https://docs.zephyrproject.org/apidoc/latest/group__condvar__apis.html

namespace Os {
namespace Zephyr {
namespace Mutex {

ZephyrConditionVariable::ZephyrConditionVariable() {
    int status = k_condvar_init(this->m_handle.m_condition);
    FW_ASSERT(status == 0, static_cast<FwAssertArgType>(status));  // If this fails, something horrible happened.
}
ZephyrConditionVariable::~ZephyrConditionVariable() {
}

ZephyrConditionVariable::Status ZephyrConditionVariable::pend(Os::Mutex& mutex) {
    ZephyrMutexHandle* mutex_handle = reinterpret_cast<ZephyrMutexHandle*>(mutex.getHandle());
    int status = k_condvar_wait(this->m_handle.m_condition, mutex_handle->m_mutex_descriptor, K_FOREVER);
    if(status == 0){
        return Status::OP_OK;
    }
}
void ZephyrConditionVariable::notify() {
    FW_ASSERT(k_condvar_signal(this->m_handle.m_condition) == 0);
}
void ZephyrConditionVariable::notifyAll() {
    int status = k_condvar_broadcast(this->m_handle.m_condition);
    Fw::Logger::log("Number of %d threads have succesfully woken up\n", status);
}

ConditionVariableHandle* ZephyrConditionVariable::getHandle() {
    return &m_handle;
}

}  // namespace Mutex
}  // namespace Zephyr
}  // namespace Os