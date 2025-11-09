# Drv::ZephyrI2cDriver

I2C driver for the fprime-zephyr OSAL layer that bridges F´ components with Zephyr's I2C API.

ZephyrI2cDriver implements the F´ `Drv.I2c` interface.

## Usage

The `ZephyrI2cDriver` requires a [Zephyr I2C device structure](https://docs.zephyrproject.org/3.7.0/doxygen/html/structdevice.html) to be provided during initialization. This structure represents the I2C bus and target address. It can be obtained through Zephyr's device tree or by using the `device_get_binding` function. 

```c++
// In Topology.cpp
#include <zephyr/device.h>

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

...

void configureTopology() {
    Drv::I2cStatus status = i2cDriver.open(i2c_dev);
    if (status != Drv::I2cStatus::I2C_OK) {
        Fw::Logger::log("Failed to open I2C device\n");
    } else {
        Fw::Logger::log("I2C device opened successfully\n");
    }
    
    ...
}
```

## Links

* [Zephyr I2C Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/i2c.html)
* [Zephyr I2C API Reference](https://docs.zephyrproject.org/latest/doxygen/html/group__i2c__interface.html)
