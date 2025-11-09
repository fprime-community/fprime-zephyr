// ======================================================================
// \title  ZephyrI2cDriver.cpp
// \author viv
// \brief  cpp file for ZephyrI2cDriver component implementation class
// Relevant Documentation:
// https://docs.zephyrproject.org/latest/doxygen/html/structi2c__dt__spec.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__i2c__interface.html#gafc6799ac7a95eaa8e186cbe6981b1548

// ======================================================================

#include "fprime-zephyr/Drv/ZephyrI2cDriver/ZephyrI2cDriver.hpp"
#include <Fw/FPrimeBasicTypes.hpp>

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrI2cDriver ::ZephyrI2cDriver(const char* const compName) : ZephyrI2cDriverComponentBase(compName) {}

ZephyrI2cDriver ::~ZephyrI2cDriver() {}

Drv::I2cStatus ZephyrI2cDriver::open(const struct device* i2c_device) {
    this->m_device = i2c_device;
    if (!device_is_ready(this->m_device)) {
        return Drv::I2cStatus::I2C_OPEN_ERR;
    }
    return Drv::I2cStatus::I2C_OK;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

Drv::I2cStatus ZephyrI2cDriver ::read_handler(FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) {
    int status = i2c_read(this->m_device, serBuffer.getData(), serBuffer.getSize(), addr);
    if (status != 0) {
        return Drv::I2cStatus::I2C_READ_ERR;
    }
    return Drv::I2cStatus::I2C_OK;
}

Drv::I2cStatus ZephyrI2cDriver ::write_handler(FwIndexType portNum, U32 addr, Fw::Buffer& serBuffer) {
    int status = i2c_write(this->m_device, serBuffer.getData(), serBuffer.getSize(), addr);
    if (status != 0) {
        return Drv::I2cStatus::I2C_WRITE_ERR;
    }
    return Drv::I2cStatus::I2C_OK;
}

Drv::I2cStatus ZephyrI2cDriver ::writeRead_handler(FwIndexType portNum,
                                                   U32 addr,
                                                   Fw::Buffer& writeBuffer,
                                                   Fw::Buffer& readBuffer) {
    int status = i2c_write_read(this->m_device, addr, writeBuffer.getData(), writeBuffer.getSize(),
                                readBuffer.getData(), readBuffer.getSize());
    if (status != 0) {
        return Drv::I2cStatus::I2C_WRITE_ERR;
    }
    return Drv::I2cStatus::I2C_OK;
}

}  // namespace Zephyr
