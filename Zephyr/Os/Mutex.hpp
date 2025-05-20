// ======================================================================
// \title Os/Zephyr/Mutex.hpp
// \brief Zephyr definitions for Os::Mutex
// ======================================================================
#ifndef OS_Zephyr_MUTEX_HPP
#define OS_Zephyr_MUTEX_HPP
#include <zephyr/kernel.h>
#include <Os/Mutex.hpp>

namespace Os {
namespace Zephyr {
namespace Mutex {

struct ZephyrMutexHandle : public MutexHandle {
    k_mutex* m_mutex_descriptor;
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

}  // namespace Mutex
}  // namespace Zephyr
}  // namespace Os
#endif  // OS_Zephyr_MUTEX_HPP