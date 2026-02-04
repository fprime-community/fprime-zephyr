// ======================================================================
// \title  ZephyrI2cDriver.hpp
// \author viv
// \brief  hpp file for ZephyrI2cDriver component implementation class
// ======================================================================

#ifndef ZephyrI2cDriver_HPP
#define ZephyrI2cDriver_HPP

#include "fprime-zephyr/Drv/ZephyrI2cDriver/ZephyrI2cDriverComponentAc.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <Os/File.hpp>

namespace Zephyr {

class ZephyrI2cDriver : public ZephyrI2cDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ZephyrI2cDriver object
    ZephyrI2cDriver(const char* const compName  //!< The component name
    );

    Drv::I2cStatus open(const struct device* i2c_device);

    //! Destroy ZephyrI2cDriver object
    ~ZephyrI2cDriver();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for write
    //!
    Drv::I2cStatus write_handler(const FwIndexType portNum, /*!< The port number*/
                                 U32 addr,
                                 Fw::Buffer& serBuffer) override;

    //! Handler implementation for read
    //!
    Drv::I2cStatus read_handler(const FwIndexType portNum, /*!< The port number*/
                                U32 addr,
                                Fw::Buffer& serBuffer) override;

    //! Handler implementation for writeRead
    //!
    Drv::I2cStatus writeRead_handler(const FwIndexType portNum, /*!< The port number*/
                                     U32 addr,
                                     Fw::Buffer& writeBuffer,
                                     Fw::Buffer& readBuffer) override;

    const struct device* m_device;  // Zephyr device handle
};

}  // namespace Zephyr

#endif
