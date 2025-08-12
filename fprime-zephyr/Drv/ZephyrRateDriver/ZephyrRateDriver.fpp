module Zephyr {

  @ An interrupt based driver for Arduino rate groups.
  passive component ZephyrRateDriver {

    import Drv.Tick

    @ GPIO output port to indicate startup error
    output port error: [1] Drv.GpioWrite
  }

}
