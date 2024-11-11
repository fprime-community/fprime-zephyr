// ======================================================================
// \title Os/Zephyr/ConditionVariable.cpp
// \brief Zephyr implementations for Os::ConditionVariable
// ======================================================================
#include "Zephyr/Os/ConditionVariable.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {
namespace Zephyr {

void ZephyrConditionVariable::wait(Os::Mutex& mutex) {
}
void ZephyrConditionVariable::notify() {
}
void ZephyrConditionVariable::notifyAll() {
}

ConditionVariableHandle* ZephyrConditionVariable::getHandle() {
    return &m_handle;
}

}  // namespace Zephyr
}  // namespace Os
