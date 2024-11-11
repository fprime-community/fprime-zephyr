// ======================================================================
// \title Os/Zephyr/ConditionVariable.hpp
// \brief Zephyr definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_ZEPHYR_CONDITION_VARIABLE_HPP
#define OS_ZEPHYR_CONDITION_VARIABLE_HPP
#include <Os/Condition.hpp>

namespace Os {
namespace Zephyr {

struct ZephyrConditionVariableHandle : public ConditionVariableHandle {};

//! \brief Zephyr implementation of Os::ConditionVariable
//!
//! Zephyr implementation of `ConditionVariable` for use as a delegate class handling error-only file operations.
//!
class ZephyrConditionVariable : public ConditionVariableInterface {
  public:
    //! \brief constructor
    //!
    ZephyrConditionVariable() = default;

    //! \brief destructor
    //!
    ~ZephyrConditionVariable() override = default;
    
    //! \brief assignment operator is forbidden
    ConditionVariableInterface& operator=(const ConditionVariableInterface& other) override = delete;

    //! \brief wait releasing mutex
    void wait(Os::Mutex& mutex) override;

    //! \brief notify a single waiter
    void notify() override;

    //! \brief notify all current waiters
    void notifyAll() override;

    //! \brief get handle
    ConditionVariableHandle* getHandle() override;

  private:
    //! Handle for PosixMutex
    ZephyrConditionVariableHandle m_handle;
};

}  // namespace Posix
}  // namespace Os
#endif  // OS_ZEPHYR_CONDITION_VARIABLE_HPP
