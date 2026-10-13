# Zephyr::LoRa

Wrapper for the [Zephyr LoRa driver](https://docs.zephyrproject.org/latest/connectivity/lora_lorawan/index.html). This will integrate into the communication stack.

### Typical Usage

This is used as a radio in the F Prime communication stack transmitting via the LoRa radio. The `run` port must be
connected to a rate group (any rate; the recovery bound is time based); it emits the recovery SUCCESS owed after a deferred
transmit, so an unconnected `run` port stalls the uplink chain after the first deferral.

### Carrier Sense

The LoRa modem is half-duplex. Before transmitting, `dataIn` reads the SX127x in-band RSSI through the loramac-node
chip driver's `SX1276ReadRssi`/`SX1272ReadRssi` (the driver Zephyr's `sx127x.c` is built on, available when
`CONFIG_HAS_SEMTECH_SX1276`/`SX1272` is set; Zephyr's own `Radio` table leaves `Radio.Rssi` NULL) and, when it exceeds `RSSI_BUSY_THRESHOLD_DBM`, returns the buffer through `dataReturnOut`
and emits `Fw::Success::FAILURE` without transmitting. RSSI sees a transmission at any point in the packet, not only
its preamble, so no modem-status or post-receive holdoff check is needed. Per the Communication Adapter Protocol the
component then owes one recovery `SUCCESS`, which `run` emits once the channel is clear or once the channel has
carried energy for `MAX_TX_DEFER_MS`, whichever comes first; a retry released at that bound is sent regardless of the
energy, so a released frame is never deferred again by the same busy episode. Exactly one `SUCCESS` is emitted per deferral, and it is armed only
after the `FAILURE` has been delivered so it can never overtake it. No events are emitted on the send path for a
deferral; deferrals are counted in the `TransmitsDeferred` channel.

Energy that persists longer than `MAX_TX_DEFER_MS` cannot be a packet: it is noise or jamming, and deferring to it
would silence the radio for as long as it lasts (or for ever, if someone floods the channel). Once a transmit has
been held off that long the frame is sent regardless of RSSI, `TransmitsForced` is incremented and the
`ChannelBusyTimeout` warning is emitted. Setting `RSSI_BUSY_THRESHOLD_DBM` to 32767 disables carrier sense entirely.
The same RSSI read is sampled on every `run` tick and telemetered as `ChannelRssi`, so operators see the current
noise floor and can set the threshold for the site. On radios without RSSI support the check compiles to false and
transmits pre-empt receives.

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
| LORA-12 | The LoRa component shall not transmit while in-band RSSI exceeds `RSSI_BUSY_THRESHOLD_DBM`; it shall return the buffer and emit `FAILURE` instead | Hardware Test |
| LORA-13 | The LoRa component shall emit exactly one recovery `SUCCESS`, from the `run` port, after each deferred transmit once the channel is no longer busy or has carried energy for `MAX_TX_DEFER_MS` |
| LORA-15 | The LoRa component shall transmit regardless of RSSI once a transmit has been held off for `MAX_TX_DEFER_MS`, counting it in `TransmitsForced` | Hardware Test |
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
| RSSI_BUSY_THRESHOLD_DBM | In-band RSSI in dBm above which the channel is busy; 32767 disables carrier sense |
| MAX_TX_DEFER_MS | Bound in ms on a channel-energy wait: a deferred transmit is released once the channel clears or this long has passed, and a transmit held this long is sent anyway |

Projects that override `zephyr-config/LoRaCfg.hpp` must define `RSSI_BUSY_THRESHOLD_DBM` and `MAX_TX_DEFER_MS`. Carrier sense
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
| TransmitsDeferred | Count of transmits deferred because the channel carried energy |
| TransmitsForced | Count of transmits sent into channel energy that outlasted MAX_TX_DEFER_MS |
| ChannelRssi | In-band RSSI sampled before each transmit and on each run tick |

## Events

| Name | Description |
|---|---|
| ConfigurationFailed | Failed to configure the LoRa radio |
| SendFailed          | Failed to send data out LoRa radio |
| AllocationFailed    | Failed to allocate buffer for received data|
| FrequencyOutOfRange | SET_FREQ frequency outside MIN_FREQUENCY to MAX_FREQUENCY |
| FrequencySet        | Carrier frequency applied by a successful SET_FREQ |
