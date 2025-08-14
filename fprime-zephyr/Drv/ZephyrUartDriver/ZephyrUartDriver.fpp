module Zephyr {

  passive component ZephyrUartDriver {
    import Drv.ByteStreamDriver

    @ Polled sched-in for reading UART
    guarded input port schedIn: Svc.Sched

    @Allocate new buffer
    output port allocate: Fw.BufferGet

    @return the allocated buffer
    output port deallocate: Fw.BufferSend
  }
}
