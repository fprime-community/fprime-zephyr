// ======================================================================
// \title Os/Zephyr/Mutex.hpp
// \brief Zephyr definitions for Os::Mutex
// ======================================================================
#include "Os/Mutex.hpp"
#include <zephyr/kernel.h>

#ifndef OS_ZEPHYR_MUTEX_HPP
#define OS_ZEPHYR_MUTEX_HPP
namespace Os {
namespace Zephyr {

struct ZephyrMutexHandle : public MutexHandle {
  struct k_mutex* m_mutex_descriptor = nullptr;  //!< Zephyr mutex handle
};

//! \brief Zephyr implementation of Os::Mutex
//!
//! Zephyr implementation of `MutexInterface` for use as a delegate class handling error-only file operations.
//!
class ZephyrMutex : public MutexInterface {
  public:
    //! \brief constructor
    //!
    ZephyrMutex();

    //! \brief destructor
    //!
    ~ZephyrMutex() override;

    //! \brief return the underlying mutex handle (implementation specific)
    //! \return internal mutex handle representation
    MutexHandle* getHandle() override;

    Status take() override;     //!<  lock the mutex and get return status
    Status release() override;  //!<  unlock the mutex and get return status

  private:
    //! Handle for ZephyrMutex
    ZephyrMutexHandle m_handle;
};

}  // namespace Zephyr
}  // namespace Os
#endif  // OS_ZEPHYR_MUTEX_HPP
