module Zephyr {
    @ Zephyr GPIO Driver
    passive component ZephyrGpioDriver {

        sync input port gpioWrite: Drv.GpioWrite
        
        sync input port gpioRead: Drv.GpioRead

    }
}