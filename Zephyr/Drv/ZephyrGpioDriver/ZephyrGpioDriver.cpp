// ======================================================================
// \title  ZephyrGpioDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrGpioDriver component implementation class
// ======================================================================

#include <Zephyr/Drv/ZephyrGpioDriver/ZephyrGpioDriver.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

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

  Os::File::Status ZephyrGpioDriver::open(struct gpio_dt_spec gpio, GpioConfiguration configuration)
  {
    this->m_pin = gpio;

    if (!gpio_is_ready_dt(&this->m_pin))
    {
      return Os::File::Status::OTHER_ERROR; 
    }

    if (gpio_pin_configure_dt(&this->m_pin, (configuration == GpioConfiguration::IN ? GPIO_INPUT : GPIO_OUTPUT)) < 0)
    {
      return Os::File::Status::OTHER_ERROR; 
    }

    return Os::File::Status::OP_OK;
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  Drv::GpioStatus ZephyrGpioDriver ::
      gpioRead_handler(
          const FwIndexType portNum,
          Fw::Logic &state)
  {
    U8 value = gpio_pin_get_dt(&this->m_pin);
    state = (value == 1) ? Fw::Logic::HIGH : Fw::Logic::LOW;
    return Drv::GpioStatus::OP_OK;
  }

  Drv::GpioStatus ZephyrGpioDriver ::
      gpioWrite_handler(
          const FwIndexType portNum,
          const Fw::Logic &state)
  {
    gpio_pin_set_dt(&this->m_pin, (state == Fw::Logic::HIGH) ? 1 : 0);
    return Drv::GpioStatus::OP_OK;
  }

} // end namespace Zephyr
