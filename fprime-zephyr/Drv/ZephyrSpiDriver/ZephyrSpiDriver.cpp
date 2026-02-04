// ======================================================================
// \title  ZephyrSpiDriver.cpp
// \author viv
// \brief  cpp file for ZephyrSpiDriver component implementation class
// Relevant Documentation:
// https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__spi__interface.html
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrSpiDriver/ZephyrSpiDriver.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include <Fw/FPrimeBasicTypes.hpp>

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrSpiDriver ::ZephyrSpiDriver(const char *const compName)
    : ZephyrSpiDriverComponentBase(compName) {}

ZephyrSpiDriver ::~ZephyrSpiDriver() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------
void ZephyrSpiDriver::configure(const struct device *device, spi_config spiConfig){
  FW_ASSERT(device != nullptr);
  // Driver only supports SPI Master mode
  FW_ASSERT(spiConfig.slave == 0);
  this->m_dev = device;
  this->m_spiConfig = spiConfig;

}
void ZephyrSpiDriver ::SpiReadWrite_handler(FwIndexType portNum,
                                            Fw::Buffer &writeBuffer,
                                            Fw::Buffer &readBuffer) {
    // Call status-variant and discard status
    (void) this->SpiWriteRead_handler(portNum, writeBuffer, readBuffer);
}

Drv::SpiStatus ZephyrSpiDriver ::SpiWriteRead_handler(FwIndexType portNum,
                                            Fw::Buffer &writeBuffer,
                                            Fw::Buffer &readBuffer) {
  if (!device_is_ready(this->m_dev)) {
    return Drv::SpiStatus::SPI_OPEN_ERR;
  }

  struct spi_config spi_config = m_spiConfig;
  // Set up write buffer
  spi_buf write_buffers[1];
  write_buffers[0].buf = writeBuffer.getData();
  write_buffers[0].len = writeBuffer.getSize();
  spi_buf_set write_buffer_set = {
      .buffers = write_buffers,
      .count = 1,
  };

  // Set up read buffer
  spi_buf read_buffers[1];
  read_buffers[0].buf = readBuffer.getData();
  read_buffers[0].len = readBuffer.getSize();
  spi_buf_set read_buffer_set = {
      .buffers = read_buffers,
      .count = 1,
  };
  Drv::SpiStatus returnStatus = Drv::SpiStatus::SPI_OK; 
  int status = spi_transceive(this->m_dev, &spi_config, &write_buffer_set, &read_buffer_set);
  if(status < 0){
    switch (status) {
      case -EINVAL: {
        returnStatus = Drv::SpiStatus::SPI_MISMATCH_ERR;
        break;
      }
      case -ENOTSUP: {
        returnStatus = Drv::SpiStatus::SPI_OPEN_ERR;
        break;
      } default: {
        returnStatus = Drv::SpiStatus::SPI_OTHER_ERR;
        break;
      }
    }
  }
  return returnStatus;
}

} // namespace Zephyr
