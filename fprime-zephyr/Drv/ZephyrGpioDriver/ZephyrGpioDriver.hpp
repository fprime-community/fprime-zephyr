// ======================================================================
// \title  ZephyrGpioDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrGpioDriver component implementation class
// ======================================================================

#ifndef ZephyrGpioDriver_HPP
#define ZephyrGpioDriver_HPP

#include "fprime-zephyr/Drv/ZephyrGpioDriver/ZephyrGpioDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <Os/File.hpp>
#include <zephyr/drivers/gpio.h>

namespace Zephyr {

  class ZephyrGpioDriver :
    public ZephyrGpioDriverComponentBase
  {

    public:

      enum GpioConfiguration {
          IN,   //!< input
          OUT,  //!< output
      };

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object ZephyrGpioDriver
      //!
      ZephyrGpioDriver(
          const char *const compName /*!< The component name*/
      );

      //! Destroy object ZephyrGpioDriver
      //!
      ~ZephyrGpioDriver();

      //! open GPIO
       Os::File::Status open(struct gpio_dt_spec gpio, GpioConfiguration configuration);

    private:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for gpioRead
      //!
      Drv::GpioStatus gpioRead_handler(
          const FwIndexType portNum, /*!< The port number*/
          Fw::Logic &state 
      );

      //! Handler implementation for gpioWrite
      //!
      Drv::GpioStatus gpioWrite_handler(
          const FwIndexType portNum, /*!< The port number*/
          const Fw::Logic &state 
      );

      struct gpio_dt_spec m_pin;
  };

} // end namespace Zephyr

#endif
