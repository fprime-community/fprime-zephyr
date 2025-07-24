// ======================================================================
// \title  ZephyrSpiDriver.hpp
// \author viv
// \brief  hpp file for ZephyrSpiDriver component implementation class
// ======================================================================

#ifndef ZephyrSpiDriver_HPP
#define ZephyrSpiDriver_HPP

#include "fprime-zephyr/Drv/ZephyrSpiDriver/ZephyrSpiDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

namespace Zephyr {

class ZephyrSpiDriver final : public ZephyrSpiDriverComponentBase {

public:
  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  //! Construct ZephyrSpiDriver object
  ZephyrSpiDriver(const char *const compName //!< The component name
  );

  //! Destroy ZephyrSpiDriver object
  ~ZephyrSpiDriver();

private:
  // ----------------------------------------------------------------------
  // Handler implementations for typed input ports
  // ----------------------------------------------------------------------

  //! Handler implementation for SpiReadWrite
  //!
  //! Port to perform a synchronous read/write operation over the SPI bus
  void SpiReadWrite_handler(FwIndexType portNum, //!< The port number
                            Fw::Buffer &writeBuffer,
                            Fw::Buffer &readBuffer) override;

  struct spi_dt_spec m_device;
};

} // namespace Zephyr

#endif
