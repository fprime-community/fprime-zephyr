// ======================================================================
// \title Os/Zephyr/ConditionVariable.hpp
// \brief Zephyr definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_ZEPHYR_CONDITION_VARIABLE_HPP
#define OS_ZEPHYR_CONDITION_VARIABLE_HPP
#include <pthread.h>
#include <Os/Condition.hpp>
#include <zephyr/kernel.h>

namespace Os {
namespace Zephyr {
namespace Mutex {

struct ZephyrConditionVariableHandle : public ConditionVariableHandle {
    k_condvar* m_condition; //! The condition variable
};

//! \brief Zephyr implementation of Os::ConditionVariable
//!
//! Zephyr implementation of `ConditionVariable` for use as a delegate class handling error-only file operations.
//!
class ZephyrConditionVariable : public ConditionVariableInterface {
  public:
    //! \brief constructor
    //!
    ZephyrConditionVariable();

    //! \brief destructor
    //!
    ~ZephyrConditionVariable() override;

    ConditionVariableInterface& operator=(const ConditionVariableInterface& other) override = delete;

    //! \brief wait releasing mutex
    ZephyrConditionVariable::Status pend(Os::Mutex& mutex) override;

    //! \brief notify a single waiter
    void notify() override;

    //! \brief notify all current waiters
    void notifyAll() override;

    //! \brief get handle
    ConditionVariableHandle* getHandle() override;

  private:
    //! Handle for ZephyrMutex
    ZephyrConditionVariableHandle m_handle;
};

}  // namespace Mutex
}  // namespace Zephyr
}  // namespace Os
#endif  // OS_ZEPHYR_CONDITION_VARIABLE_HPP