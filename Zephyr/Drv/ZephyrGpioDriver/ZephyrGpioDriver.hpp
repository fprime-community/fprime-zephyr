// ======================================================================
// \title  ZephyrGpioDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrGpioDriver component implementation class
// ======================================================================

#ifndef ZephyrGpioDriver_HPP
#define ZephyrGpioDriver_HPP

#include "Zephyr/Drv/ZephyrGpioDriver/ZephyrGpioDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

namespace Zephyr {

  class ZephyrGpioDriver :
    public ZephyrGpioDriverComponentBase
  {

    public:

      enum GpioDirection {
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
      bool open(struct gpio_dt_spec gpio, GpioDirection direction);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for gpioRead
      //!
      void gpioRead_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          Fw::Logic &state 
      );

      //! Handler implementation for gpioWrite
      //!
      void gpioWrite_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          const Fw::Logic &state 
      );

      struct gpio_dt_spec m_pin;
    };

} // end namespace Zephyr

#endif
