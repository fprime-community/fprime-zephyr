// ======================================================================
// \title Os/Posix/DefaultMutex.cpp
// \brief sets default Os::Mutex Posix implementation via linker
// ======================================================================
#include "Os/Delegate.hpp"
#include "Zephyr/Os/ConditionVariable.hpp"
#include "Zephyr/Os/Mutex.hpp"
namespace Os {

//! \brief get a delegate for MutexInterface that intercepts calls for Posix
//! \param aligned_new_memory: aligned memory to fill
//! \return: pointer to delegate
MutexInterface* MutexInterface::getDelegate(MutexHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<MutexInterface, Os::Zephyr::Mutex::ZephyrMutex>(aligned_new_memory);
}

//! \brief get a delegate for MutexInterface that intercepts calls for Posix
//! \param aligned_new_memory: aligned memory to fill
//! \return: pointer to delegate
ConditionVariableInterface* ConditionVariableInterface::getDelegate(
    ConditionVariableHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<ConditionVariableInterface, Os::Zephyr::Mutex::ZephyrConditionVariable,
                                      ConditionVariableHandleStorage>(aligned_new_memory);
}
}  // namespace Os