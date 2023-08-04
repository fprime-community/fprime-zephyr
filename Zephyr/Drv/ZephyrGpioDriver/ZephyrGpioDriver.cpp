// ======================================================================
// \title  ZephyrGpioDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrGpioDriver component implementation class
// ======================================================================

#include <Zephyr/Drv/ZephyrGpioDriver/ZephyrGpioDriver.hpp>
#include <FpConfig.hpp>

namespace Zephyr
{

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  ZephyrGpioDriver ::
      ZephyrGpioDriver(
          const char *const compName) : ZephyrGpioDriverComponentBase(compName)
  {
  }

  ZephyrGpioDriver ::
      ~ZephyrGpioDriver()
  {
  }

  bool ZephyrGpioDriver::open(struct gpio_dt_spec gpio, GpioDirection direction)
  {
    this->m_pin = gpio;

    if (!gpio_is_ready_dt(&this->m_pin))
    {
      return false;
    }

    if (gpio_pin_configure_dt(&this->m_pin, (direction == GpioDirection::IN ? GPIO_INPUT : GPIO_OUTPUT)) < 0)
    {
      return false;
    }

    return true;
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void ZephyrGpioDriver ::
      gpioRead_handler(
          const NATIVE_INT_TYPE portNum,
          Fw::Logic &state)
  {
    U8 value = gpio_pin_get_dt(&this->m_pin);
    state = (value == 1) ? Fw::Logic::HIGH : Fw::Logic::LOW;
  }

  void ZephyrGpioDriver ::
      gpioWrite_handler(
          const NATIVE_INT_TYPE portNum,
          const Fw::Logic &state)
  {
    gpio_pin_set_dt(&this->m_pin, (state == Fw::Logic::HIGH) ? 1 : 0);
  }

} // end namespace Zephyr
