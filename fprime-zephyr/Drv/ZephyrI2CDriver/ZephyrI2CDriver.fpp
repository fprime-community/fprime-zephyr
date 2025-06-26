module Zephyr {
    @ I²C driver for the fprime-zephyr OSAL layer that bridges F´ components with Zephyr's I²C API.
    passive component ZephyrI2CDriver {
        import Drv.I2c
    }
}