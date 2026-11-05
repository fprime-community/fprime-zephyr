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
  @ Transmit: `send` queues the whole frame in a TX ring drained by the interrupt (or rejects
  @ it with OTHER_ERROR when it does not fit); `ready` is re-signalled once the ring recovers.
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

    @ A frame passed to `send` did not fit in the free space of the TX ring and
    @ was rejected whole (OTHER_ERROR returned to the caller, nothing written to
    @ the device). `ready` is signalled again once TX_RESUME_THRESHOLD bytes of
    @ the ring are free. Reported from task context; the count is cumulative.
    event TxFrameDropped(
      frameSize: U32 @< Size in bytes of the most recently rejected frame
      totalDrops: U32 @< Cumulative number of rejected frames
    ) \
      severity warning low \
      format "UART TX frame of {} bytes dropped, {} drop(s) total" \
      throttle 5


    ###############################################################################
    # Telemetry                                                                   #
    ###############################################################################
    @ Cumulative bytes received from the UART into the RX ring
    telemetry RxBytes: U32 update on change


    @ Cumulative RX overrun indications (hardware overrun or ring put failure)
    telemetry RxOverrunCount: U32 update on change

    @ Cumulative number of times the RX interrupt was paused because the RX ring
    @ was full (back-pressure engaged)
    telemetry RxBackpressureCount: U32 update on change

    @ Cumulative number of drain iterations that stalled because no Fw.Buffer
    @ could be allocated (data stays in the ring and is retried)
    telemetry RxAllocFailCount: U32 update on change

    @ Cumulative bytes moved from the TX ring into the device
    telemetry TxBytes: U32 update on change

    @ Cumulative frames rejected by `send` because they did not fit in the TX ring
    telemetry TxDropCount: U32 update on change

  }
}
