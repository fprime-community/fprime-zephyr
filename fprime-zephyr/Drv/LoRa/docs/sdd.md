# Zephyr::LoRa

Wrapper for the [Zephyr LoRa driver](https://docs.zephyrproject.org/latest/connectivity/lora_lorawan/index.html). This will integrate into the communication stack.

### Typical Usage

This is used as a radio in the F Prime communication stack transmitting via the LoRa radio. The `run` port must be
connected to a rate group (any rate; the recovery bound is time based); it emits the recovery SUCCESS owed after a deferred
transmit, so an unconnected `run` port stalls the uplink chain after the first deferral.

### Receive-Aware Transmit

The LoRa modem is half-duplex. Before transmitting, `dataIn` reads the SX127x `RegModemStat` register (via the
loramac-node HAL, available when `CONFIG_HAS_SEMTECH_SX1276`/`SX1272` is set) and, when a packet reception is in
progress, returns the buffer through `dataReturnOut` and emits `Fw::Success::FAILURE` without transmitting, so the
in-flight receive is not aborted. The channel is also treated as busy for `RX_HOLDOFF_MS` after the last received
packet, so a transmit cannot key into the short gap between frames of an uplink burst, where the next frame's
preamble is not yet detectable by `RegModemStat`. Per the Communication Adapter Protocol the component then owes one recovery
`SUCCESS`, which `run` emits once the channel is no longer busy or after `DEFERRED_TX_RECOVERY_MS`, whichever comes
first. Exactly one `SUCCESS` is emitted per deferral, and it is armed only after the
`FAILURE` has been delivered so it can never overtake it. No events are emitted on the send path; deferrals are
counted in the `TransmitsDeferred` channel. On radios without modem status support the check compiles to false and
transmits pre-empt receives as before.

The recovery `SUCCESS` is emitted on the rate-group thread. An upstream `Svc.ComRetry` should therefore be
configured with `recover_on_sender_thread = true` so the resend (and the blocking `lora_send`) is pulled back onto
the sending thread rather than executed on the rate group.

## Requirements

| Name | Description | Validation |
|---------|---|---|
| LORA-01 | The LoRa component shall interface with the Zephyr Lora driver | INSPECTION |
| LORA-02 | The LoRa component shall provide DATA_RATE and CODING_RATE as paramaters | Unit-Test |
| LORA-03 | The LoRa component shall provide FREQUENCY, MIN_FREQUENCY, MAX_FREQUENCY, BANDWITH, TRANSMIT_POWER, and PREAMBLE as configurable parameters | Unit-Test |
| LORA-04 | The LoRa component shall telemeter BytesSent and BytesReceived channels | Unit-Test |
| LORA-05 | The LoRa component shall support the Svc.Com interface | Unit-Test |
| LORA-06 | The LoRa component shall have a continuous wave command | Unit-Test |
| LORA-07 | The LoRa component shall wrap the Zephyr LoRa driver | Unit-Test |
| LORA-08 | The LoRa component shall configure the Zephyr LoRa driver for transmit only when sending data | Unit-Test |
| LORA-09 | The LoRa component shall provide a command to change the carrier frequency, rejecting frequencies outside the configured MIN_FREQUENCY to MAX_FREQUENCY range | Inspection |
| LORA-10 | The LoRa component shall expose enableTransmit and disableTransmit ports to control transmission | Unit-Test |
| LORA-11 | The TRANSMIT command and enableTransmit/disableTransmit ports shall enable or disable the com-status ping-pong | Unit-Test |
| LORA-12 | The LoRa component shall not abort a packet reception in progress in order to transmit; it shall return the buffer and emit `FAILURE` instead | Hardware Test |
| LORA-13 | The LoRa component shall emit exactly one recovery `SUCCESS`, from the `run` port, after each deferred transmit once the channel is no longer busy or `DEFERRED_TX_RECOVERY_MS` elapses |
| LORA-15 | The LoRa component shall treat the channel as busy for `RX_HOLDOFF_MS` after the last received packet | Hardware Test | Hardware Test |
| LORA-14 | The LoRa component shall not emit events on the send path for deferrals; it shall telemeter the `TransmitsDeferred` count | Inspection |


## Port Interfaces

| Name | Description |
|---|---|
| Svc.Com | Interface to plug the radio into the communication stack |
| Svc.BufferAllocation | Buffer allocation interface for received data |
| run | Rate group input; must be connected. Re-arms receive once a continuous wave ends and emits the recovery SUCCESS owed for a deferred transmit |
| enableTransmit | `Fw.Signal` input: enable LoRa transmission (starts com-status ping-pong) |
| disableTransmit | `Fw.Signal` input: disable LoRa transmission (stops ping-pong via `DISABLING`) |
| loraFirstStart | `Fw.Signal` output: emitted once, the first time transmit is enabled |


## Configuration

| Name | Description |
|------|---|
| FREQUENCY   | Frequency of the radio transmission / receive at boot |
| MIN_FREQUENCY | Lowest frequency accepted by SET_FREQ |
| MAX_FREQUENCY | Highest frequency accepted by SET_FREQ. Defaults to FREQUENCY, as does MIN_FREQUENCY, which disables retuning |
| BANDWIDTH   | Number of parity bits sent             |
| TX_POWER    | Transmission power of the raio |
| PREAMBLE    | Preamble length in bytes |
| RX_HOLDOFF_MS | Time in ms after the last received packet during which transmits are deferred |
| DEFERRED_TX_RECOVERY_MS | Maximum time in ms a deferred transmit waits for the channel to clear before the recovery SUCCESS is forced |

Projects that override `zephyr-config/LoRaCfg.hpp` must define `RX_HOLDOFF_MS` and `DEFERRED_TX_RECOVERY_MS`. Receive detection
requires the loramac-node module (`ZEPHYR_LORAMAC_NODE_MODULE_DIR`) and an SX127x radio.

Projects that override `LoRaCfg.hpp` must define MIN_FREQUENCY and MAX_FREQUENCY, and FREQUENCY must lie between them (checked at compile time). Setting both to FREQUENCY keeps SET_FREQ disabled.

> [!WARNING]
> SET_FREQ moves both uplink and downlink. A wrong in-range frequency loses the link until the ground station follows it or the radio reboots back to FREQUENCY.

## Command

| Name | Description |
|------|---|
| CONTINUOUS_WAVE | Start a continuous wave for a supplied duration and respond immediately. Receive is re-armed by `run` once the wave ends; transmissions are dropped and further `CONTINUOUS_WAVE` commands return BUSY until then. The wave is transmitted at the current frequency |
| SET_FREQ | Retune receive, transmit, and continuous wave to the supplied frequency in Hz. Returns VALIDATION_ERROR outside MIN_FREQUENCY to MAX_FREQUENCY, BUSY during a continuous wave, and EXECUTION_ERROR (keeping and re-arming receive at the previous frequency) if the driver reports an error. The driver does not check the radio's RF band, so MIN_FREQUENCY to MAX_FREQUENCY must lie within it. Not persisted: the radio returns to FREQUENCY on reboot. On SX126x radios, whether image calibration is rerun for a different band depends on the Zephyr driver backend |
| TRANSMIT | Enable/disable transmission (com-status ping-pong). Runtime state may briefly be `DISABLING` while an in-flight send completes. |

## Parameters

| Name | Description |
|------|---|
| DATA_RATE    | Spreading factor / data rate for radio |
| CODING_RATE  | Number of parity bits sent             |
| BANDWIDTH_TX | Bandwidth used when transmitting       |
| BANDWIDTH_RX | Bandwidth used when receiving          |

## Telemetry

| Name | Description |
|---|---|
| BytesSent     | Total bytes sent |
| BytesReceived | Total bytes received |
| LastRssi      | RSSI value of last receive |
| LastSnr       | SNR value of last receive  |
| TransmitsDeferred | Count of transmits deferred because a receive was in progress |

## Events

| Name | Description |
|---|---|
| ConfigurationFailed | Failed to configure the LoRa radio |
| SendFailed          | Failed to send data out LoRa radio |
| AllocationFailed    | Failed to allocate buffer for received data|
| FrequencyOutOfRange | SET_FREQ frequency outside MIN_FREQUENCY to MAX_FREQUENCY |
| FrequencySet        | Carrier frequency applied by a successful SET_FREQ |
