module Zephyr {

  @ Reboots into the bootloader when the host sets a monitored UART (e.g. USB CDC ACM) to the "touch" baud rate
  passive component ZephyrTouchReset {

    @ Rate group port used to poll the UART line coding
    sync input port run: Svc.Sched

  }

}
