// ======================================================================
// \title  ZephyrI2CDriver.cpp
// \author viv
// \brief  cpp file for ZephyrI2CDriver component implementation class
// Relevant Documentation:
// https://docs.zephyrproject.org/latest/doxygen/html/structi2c__dt__spec.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__i2c__interface.html#gafc6799ac7a95eaa8e186cbe6981b1548

// ======================================================================

#include "fprime-zephyr/Drv/ZephyrI2CDriver/ZephyrI2CDriver.hpp"
#include <Fw/FPrimeBasicTypes.hpp>

namespace Zephyr {

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------


    ZephyrI2CDriver ::ZephyrI2CDriver(const char *const compName)
        : ZephyrI2CDriverComponentBase(compName) {}

    ZephyrI2CDriver ::~ZephyrI2CDriver() {}

    //  I2CConfiguration configuration ? 
    Os::File::Status ZephyrI2CDriver::open(const struct i2c_dt_spec i2c_device){
      this->m_device = i2c_device;
      if(!i2c_is_ready_dt(&this->m_device)){
        return Os::File::Status::NOT_OPENED;
      }

      return Os::File::Status::OP_OK;
    }

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    Drv::I2cStatus ZephyrI2CDriver ::read_handler(FwIndexType portNum, U32 addr,
                                                  Fw::Buffer &serBuffer) {
      int status = i2c_read_dt(&this->m_device, serBuffer.getData(), serBuffer.getSize()); 
      if(status != 0){
        printk("I2C read error\n");
      }
      return Drv::I2cStatus::I2C_OK;
    }

    Drv::I2cStatus ZephyrI2CDriver ::write_handler(FwIndexType portNum, U32 addr,
                                                  Fw::Buffer &serBuffer) {
      int status = i2c_write_dt(&this->m_device, serBuffer.getData(), serBuffer.getSize()); 
      if(status != 0){
        printk("I2C write error\n");
      }
      return Drv::I2cStatus::I2C_OK;
    }

    Drv::I2cStatus ZephyrI2CDriver ::writeRead_handler(FwIndexType portNum, U32 addr,
      Fw::Buffer &writeBuffer, Fw::Buffer &readBuffer) {
      int status = i2c_write_read_dt(&this->m_device, writeBuffer.getData(), writeBuffer.getSize(), readBuffer.getData(), readBuffer.getSize()); 
      if(status != 0){
          printk("I2C write error\n");
      }
      return Drv::I2cStatus::I2C_OK;
    }

} // namespace Zephyr
