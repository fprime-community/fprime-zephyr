// ======================================================================
// \title  ZephyrTime.hpp
// \author mstarch
// \brief  hpp file for ZephyrTime component implementation class
// ======================================================================

#ifndef Zephyr_ZephyrTime_HPP
#define Zephyr_ZephyrTime_HPP

#include "fprime-zephyr/Svc/ZephyrTime/ZephyrTimeComponentAc.hpp"

namespace Zephyr {

class ZephyrTime final : public ZephyrTimeComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ZephyrTime object
    ZephyrTime(const char* const compName  //!< The component name
    );

    //! Destroy ZephyrTime object
    ~ZephyrTime();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for timeGetPort
    //!
    //! Port to retrieve time
    void timeGetPort_handler(FwIndexType portNum,  //!< The port number
                             Fw::Time& time        //!< Reference to Time object
                             ) override;
};

}  // namespace Zephyr

#endif
