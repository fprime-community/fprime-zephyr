module Zephyr {

  @ Byte-stream driver for a Zephyr UART (hardware UART or USB CDC ACM).
  @
  @ Receive: the UART interrupt callback copies bytes from the device FIFO into
  @ a software ring buffer and applies back-pressure (disables the RX interrupt)
  @ when the ring is full instead of discarding bytes. The ring is drained into
  @ allocated Fw.Buffers and delivered on `recv` either by the rate-group
  @ `schedIn` call (default) or, when `start()` is called from the topology, by
  @ a dedicated task woken directly by the interrupt. Exactly one of the two
  @ drains the ring.
  @
  @ Transmit: `send` writes the frame synchronously with `uart_poll_out`.
  passive component ZephyrUartDriver {
    import Drv.ByteStreamDriver

    @ Polled sched-in: drains the RX ring (unless the RX task is running) and
    @ publishes telemetry/events
    guarded input port schedIn: Svc.Sched

    @ Allocate new buffer
    output port allocate: Fw.BufferGet

    @ Return the allocated buffer
    output port deallocate: Fw.BufferSend

    ###############################################################################
    # Standard AC Ports: Required for Events and Telemetry                       #
    ###############################################################################
    @ Port for requesting the current time
    time get port timeCaller

    @ Port for sending textual representation of events
    text event port logTextOut

    @ Port for sending events to downlink
    event port logOut

    @ Port for sending telemetry channels to downlink
    telemetry port tlmOut

    ###############################################################################
    # Events                                                                      #
    ###############################################################################
    @ Receive data was lost: the UART reported a hardware overrun (device FIFO
    @ overflowed while the RX interrupt was paused or masked) or the RX ring
    @ rejected data. Reported from task context; the count is cumulative.
    event RxOverrun(
      totalOverruns: U32 @< Cumulative number of overrun indications
    ) \
      severity warning low \
      format "UART RX overrun, {} overrun(s) total" \
      throttle 5


    ###############################################################################
    # Telemetry                                                                   #
    ###############################################################################
    @ Cumulative bytes received from the UART into the RX ring
    telemetry RxBytes: FwSizeType update on change


    @ Cumulative RX overrun indications (hardware overrun or ring put failure)
    telemetry RxOverrunCount: U32 update on change

    @ Cumulative number of times the RX interrupt was paused because the RX ring
    @ was full (back-pressure engaged)
    telemetry RxBackpressureCount: U32 update on change

    @ Cumulative number of drain iterations that stalled because no Fw.Buffer
    @ could be allocated (data stays in the ring and is retried)
    telemetry RxAllocFailCount: U32 update on change

  }
}
