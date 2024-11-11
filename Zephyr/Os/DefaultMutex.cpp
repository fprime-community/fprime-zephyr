// ======================================================================
// \title Os/Zephyr/DefaultMutex.cpp
// \brief sets default Os::Mutex to no-op Zephyr implementation via linker
// ======================================================================
#include "Zephyr/Os/Mutex.hpp"
#include "Zephyr/Os/ConditionVariable.hpp"
#include "Os/Delegate.hpp"
namespace Os {

//! \brief get a delegate for MutexInterface that intercepts calls for Zephyr file usage
//! \param aligned_new_memory: aligned memory to fill
//! \param to_copy: pointer to copy-constructor input
//! \return: pointer to delegate
MutexInterface *MutexInterface::getDelegate(MutexHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<MutexInterface, Os::Zephyr::ZephyrMutex>(
            aligned_new_memory
    );
}

//! \brief get a delegate for condition variable
//! \param aligned_new_memory: aligned memory to fill
//! \return: pointer to delegate
ConditionVariableInterface *ConditionVariableInterface::getDelegate(ConditionVariableHandleStorage& aligned_new_memory) {
    return Os::Delegate::makeDelegate<ConditionVariableInterface, Os::Zephyr::ZephyrConditionVariable, ConditionVariableHandleStorage>(
        aligned_new_memory
    );
}
}
