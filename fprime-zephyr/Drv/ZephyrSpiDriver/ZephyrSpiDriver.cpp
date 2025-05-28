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

void ZephyrSpiDriver ::SpiReadWrite_handler(FwIndexType portNum,
                                            Fw::Buffer &writeBuffer,
                                            Fw::Buffer &readBuffer) {
  int status = spi_transceive_dt(&this->m_device, writeBuffer, readBuffer); 
  if(status <= 0){
    printk("SPI read/write error\n");
  }
  printk("SPI read/write success\n");
}

} // namespace Zephyr
