# Zephyr::ZephyrUartDriver

Byte-stream driver (`Drv.ByteStreamDriver`) for a Zephyr UART device. The device may be a hardware UART or a USB CDC ACM
instance (`zephyr,cdc-acm-uart`); only the generic Zephyr interrupt-driven UART API is used, so the component is
device-agnostic.

### Typical Usage

The component is the `comDriver` at the bottom of the F Prime communication stack (`Svc::ComStub` -> `ZephyrUartDriver`).
The topology calls `configure(dev, baud)` once at start-up. Received bytes are delivered on `recv` in `Fw.Buffer`s obtained
from the `allocate` port (typically `Svc::BufferManager`) and returned via `recvReturnIn`/`deallocate`.

```cpp
comDriver.configure(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), 115200);
// Optional: event-driven receive instead of rate-group polling
comDriver.start(6 /* priority */, 4096 /* stack */);
```

## Requirements

| Name | Description | Validation |
|---|---|---|
| UART-RX-01 | The UART interrupt callback shall read from the device FIFO only while the software RX ring buffer has free space and shall disable the RX interrupt when the ring is full, instead of discarding bytes. | Unit test (host-side, mocked Zephyr API) |
| UART-RX-02 | The driver shall re-enable the RX interrupt from task context once the RX ring has at least `RX_CHUNK_SIZE` bytes free. | Unit test |
| UART-RX-03 | A single drain shall deliver the whole RX ring contents, in order and without loss, in chunks of at most `RX_CHUNK_SIZE` bytes, bounded by `MAX_DRAIN_ITERATIONS` deliveries. | Unit test |
| UART-RX-04 | When the allocator returns an empty buffer the drain shall leave the data in the ring, shall not return the empty buffer, and shall retry on the next drain. | Unit test |
| UART-RX-05 | When `start()` has been called successfully, a dedicated task woken by the interrupt shall be the only consumer of the RX ring; `schedIn` shall not read the ring. Otherwise `schedIn` shall drain the ring. | Unit test |
| UART-RX-06 | Hardware overrun indications (`uart_err_check`) and ring put failures shall be counted and reported by a throttled event and telemetry from task context. The interrupt callback shall not log. | Unit test / inspection |
| UART-RX-07 | All storage shall be in-class; no dynamic allocation shall occur after `start()`. Sizes are compile-time configuration; task priority and stack are supplied by the topology. | Inspection |
| UART-RX-08 | `start()` shall not assert on a missing or not-ready device; it shall return a status and leave the rate-group drain in place. A device without the interrupt-driven UART API shall be reported via `Fw::Logger` at `configure()`. | Unit test |
| UART-TX-01 | `send` shall copy the whole frame into the software TX ring and return `OP_OK`, or, when the frame does not fit in the ring's free space, write nothing, count the drop and return `OTHER_ERROR`. No partial frame shall ever reach the device. | Unit test |
| UART-TX-02 | The UART interrupt callback shall move TX ring bytes into the device FIFO with `uart_fifo_fill` and shall disable the TX interrupt when the ring is empty. `send` shall not block on the device. On a device without the interrupt-driven API, `send` shall fall back to the polled write. | Unit test |
| UART-TX-03 | After a frame was rejected, the driver shall signal `ready` once, from task context, when the TX ring has at least `TX_RESUME_THRESHOLD` bytes free. | Unit test |
| UART-TX-04 | Rejected frames shall be counted and reported by a throttled event and telemetry from task context. | Unit test |

## Design

### Receive path

```
   interrupt / CDC ACM work-queue context             task context (exactly one consumer)
 device FIFO --uart_fifo_read--> RX ring (ring_buf) --ring_buf_get--> allocate -> recv -> ... -> recvReturnIn -> deallocate
   ring full: uart_irq_rx_disable(), m_rxPaused=1              after each chunk and after the drain:
   bytes received: k_sem_give(m_wakeSem) (task mode)             if m_rxPaused && free >= RX_CHUNK_SIZE: uart_irq_rx_enable()
   uart_err_check() & UART_ERROR_OVERRUN -> m_rxOverruns++
```

`serial_cb` (registered with `uart_irq_callback_user_data_set`, `user_data = this`) claims contiguous space in the ring with
`ring_buf_put_claim` and reads directly into it with `uart_fifo_read`, so bytes are moved in bulk rather than one at a time.
When no space can be claimed the RX interrupt is disabled. For a hardware UART the device FIFO then fills and the device
reports an overrun if the peer keeps sending (unless hardware flow control is used); for CDC ACM the class driver stops
posting USB OUT requests once its own FIFO fills, so the host is NAKed and no bytes are lost — true end-to-end flow control.

`drainRx()` runs in task context and is the single consumer. For each iteration it allocates a buffer of
`min(available, RX_CHUNK_SIZE)` bytes, copies that many bytes out of the ring, re-enables the RX interrupt if it was paused
and the ring now has room, and emits the buffer on `recv`. If the allocator returns an empty buffer (e.g.
`Svc::BufferManager` exhausted) the data stays in the ring and `RxAllocFailCount` is incremented; back-pressure then
propagates to the device/host. The loop is bounded by `MAX_DRAIN_ITERATIONS = 2 * RX_RING_SIZE / RX_CHUNK_SIZE + 1`.

Two consumer modes, selected by the topology:

| Mode | Trigger | `schedIn` behavior |
|---|---|---|
| Rate-group (default) | `schedIn` | `drainRx()`, then telemetry/events |
| Event-driven (`start()` called) | `k_sem` given by `serial_cb`; `RX_TASK_WAKE_TIMEOUT_MS` safety-net timeout | telemetry/events and `k_sem_give` only — never reads the ring |

`start(priority, stackSize)` creates an `Os::Task` ("UartRx"). `m_taskStarted` is set before the task is created so there is
never a window with two readers. If the task cannot be started (e.g. the Zephyr thread stack pool is exhausted or
`stackSize` exceeds `CONFIG_DYNAMIC_THREAD_STACK_SIZE`), the driver logs with `Fw::Logger`, clears `m_taskStarted` so the
rate-group drain continues, and returns the `Os::Task::Status` to the caller; `start()` called before a successful
`configure()` returns `INVALID_HANDLE` without creating a task. `stop()`/`join()` are provided for teardown.

Back-pressure release: the drain re-enables the RX interrupt after every delivered chunk and once more after the loop. If
the interrupt pauses the ring in the instant between that final check and the end of the drain, RX stays paused until the
next drain: at most one rate-group period in rate-group mode, at most `RX_TASK_WAKE_TIMEOUT_MS` (100 ms) in event-driven
mode (the wake that paused the ring also gave the semaphore, so in practice the task re-runs immediately).

Throughput: rate-group mode delivers up to `RX_RING_SIZE` bytes per `schedIn` (e.g. 1 KiB x 10 Hz = ~10 KiB/s, versus
64 B x 10 Hz = 640 B/s before this change); event-driven mode is bounded by the device, the allocator and the downstream
chain rather than by the rate group.

The RX task executes the synchronous downstream chain (`ComStub -> FrameAccumulator -> deframer -> router`) exactly as the
rate-group thread does in the default mode, so a stack equal to the rate group's is sufficient.

### Transmit path

`send` (guarded, so the single producer of the TX ring) copies the frame into the TX ring with `ring_buf_put` and calls
`uart_irq_tx_enable()`; the cost to the caller is one bounded memcpy. The interrupt callback, when `uart_irq_tx_ready()`,
claims contiguous ring regions and hands them to `uart_fifo_fill()` (bounded by `MAX_ISR_CLAIMS` per interrupt), finishing
each claim with the count the device accepted; when the ring is empty it disables the TX interrupt. Both `uart_pl011` and
`usbd_cdc_acm` implement this API, so hardware UARTs and CDC ACM behave the same.

If the device has no interrupt-driven API (`uart_irq_callback_user_data_set()` failed at `configure()`), the ring could
never drain, so `send` keeps the previous polled, blocking `uart_poll_out` write for that device.

Whole frame or nothing: if the frame does not fit in `ring_buf_space_get()`, `send` writes nothing, records the size,
increments the drop count, sets a stalled flag and returns `OTHER_ERROR`. The caller keeps ownership of the buffer, so the
frame is not lost by the driver; what happens to it is decided upstream: `Svc::ComStub` turns `OTHER_ERROR` into a
`FAILURE` com-status, `Svc::ComQueue` stops sending, and `Svc::ComRetry` (if present in the chain) holds the frame and
resends it on the next `SUCCESS`. That `SUCCESS` comes from the driver: on each `schedIn`, if the stalled flag is set and
the ring has at least `TX_RESUME_THRESHOLD` bytes free, the driver clears the flag and calls `ready` (ComStub's
`drvConnected`), once. `TX_RESUME_THRESHOLD` must therefore be at least the largest frame the deployment sends, or the
retried frame is rejected again. Recovery latency is one rate-group period after the ring has drained.

This replaces the previous `uart_poll_out` loop, which blocked the caller for the duration of the frame on hardware UARTs
(~87 us/byte at 115200) and, on CDC ACM with the host not reading, silently discarded bytes once the 1 KiB class-driver
FIFO was full (`cdc_acm_poll_out` with flow control off), delivering partial frames to the host while returning `OP_OK`.

### Events and telemetry

Events and telemetry are emitted only from `schedIn` (task context). Counters incremented by the interrupt callback are
`std::atomic<U32>`: 32-bit atomics are lock-free on Cortex-M, whereas a 64-bit `std::atomic<FwSizeType>` would need
libatomic (not linked by Zephyr) and would take a lock inside the ISR.

## Port Interfaces

| Name | Description |
|---|---|
| `Drv.ByteStreamDriver` | `recv`, `recvReturnIn`, `send`, `ready` |
| `schedIn` | `Svc.Sched`, guarded. Drains the RX ring (rate-group mode) and publishes telemetry/events |
| `allocate` | `Fw.BufferGet`: buffers for received data |
| `deallocate` | `Fw.BufferSend`: return of received-data buffers |
| `timeCaller`, `logOut`, `logTextOut`, `tlmOut` | Standard time/event/telemetry ports |

## Configuration

Compile-time constants in `default/zephyr-config/ZephyrUartDriverCfg.hpp` (override by providing the header in a project
configuration directory):

| Name | Default | Description |
|---|---|---|
| `RX_RING_SIZE` | 1024 | RX ring buffer size in bytes |
| `RX_CHUNK_SIZE` | 256 | Maximum bytes per `allocate`/`recv` delivery |
| `RX_TASK_WAKE_TIMEOUT_MS` | 100 | Safety-net wake period of the optional RX task |
| `TX_RING_SIZE` | 2048 | TX ring buffer size in bytes; a frame larger than the free space is rejected whole |
| `TX_RESUME_THRESHOLD` | 1024 | Free TX ring bytes required to signal `ready` after a rejected frame; must be >= the largest frame sent |

Run-time: `configure(dev, baud)`; optional `start(priority, stackSize)`.

## Events

| Name | Severity | Description |
|---|---|---|
| `RxOverrun` | WARNING_LO, throttle 5 | Hardware overrun indication or ring put failure; cumulative count |
| `TxFrameDropped` | WARNING_LO, throttle 5 | A frame did not fit in the TX ring and was rejected whole; size of the latest, cumulative count |

## Telemetry

| Name | Type | Description |
|---|---|---|
| `RxBytes` | U32 | Cumulative bytes moved from the device into the RX ring |
| `RxOverrunCount` | U32 | Cumulative overrun indications |
| `RxBackpressureCount` | U32 | Times the RX interrupt was paused because the ring was full |
| `RxAllocFailCount` | U32 | Drain iterations that stalled for lack of a buffer |
| `TxBytes` | U32 | Cumulative bytes moved from the TX ring into the device |
| `TxDropCount` | U32 | Cumulative frames rejected by `send` |

## Memory

Per instance: `RX_RING_SIZE` bytes of RX ring storage (unchanged from the previous implementation), `TX_RING_SIZE` bytes
of TX ring storage (new), a `k_sem`, an `Os::Task` object and a few atomics. The optional RX task stack comes from the Zephyr dynamic thread stack pool
(`CONFIG_DYNAMIC_THREAD_POOL_SIZE` x `CONFIG_DYNAMIC_THREAD_STACK_SIZE`) and is only consumed when `start()` is called.

## Change Log

| Date | Description |
|---|---|
| 2026-10 | ISR back-pressure, bounded full-ring/event-driven RX drain, overrun event and telemetry, `start()` RX task |
| 2026-10 | Interrupt-driven TX ring: non-blocking `send`, whole-frame reject with `OTHER_ERROR`, `ready` recovery at `TX_RESUME_THRESHOLD`, drop event and telemetry |
