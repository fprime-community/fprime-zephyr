// ======================================================================
// \title  ZephyrI2CDriver.hpp
// \author viv
// \brief  hpp file for ZephyrI2CDriver component implementation class
// ======================================================================

#ifndef ZephyrI2CDriver_HPP
#define ZephyrI2CDriver_HPP

#include "fprime-zephyr/Drv/ZephyrI2CDriver/ZephyrI2CDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <Os/File.hpp>

namespace Zephyr {

  class ZephyrI2CDriver :
    public ZephyrI2CDriverComponentBase
  {
    public:
        // ----------------------------------------------------------------------
        // Component construction and destruction
        // ----------------------------------------------------------------------

        //! Construct ZephyrI2CDriver object
        ZephyrI2CDriver(const char *const compName //!< The component name
        );

        Os::File::Status open(struct i2c_dt_spec i2c_device);

        //! Destroy ZephyrI2CDriver object
        ~ZephyrI2CDriver();

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

        struct i2c_dt_spec m_device; // has a pointer to I2C bus, and target address (U16)
  };

} // namespace Zephyr

#endif
