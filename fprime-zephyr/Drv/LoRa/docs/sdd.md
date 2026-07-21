# Zephyr::LoRa

Wrapper for the [Zephyr LoRa driver](https://docs.zephyrproject.org/latest/connectivity/lora_lorawan/index.html). This will integrate into the communication stack.

### Typical Usage

This is used as a radio in the F Prime communication stack transmitting via the LoRa radio.

## Requirements

| Name | Description | Validation |
|---------|---|---|
| LORA-01 | The LoRa component shall interface with the Zephyr Lora driver | INSPECTION |
| LORA-02 | The LoRa component shall provide DATA_RATE and CODING_RATE as paramaters | Unit-Test |
| LORA-03 | The LoRa component shall provide FREQUENCY, MIN_FREQUENCY, MAX_FREQUENCY, BANDWITH, TRANSMIT_POWER, and PREAMBLE as configurable parameters | Unit-Test |
| LORA-04 | The LoRa component shall telemeter BytesSent and BytesRecv channels | Unit-Test |
| LORA-05 | The LoRa component shall support the Svc.Com interface | Unit-Test |
| LORA-06 | The LoRa component shall have a continuous wave command | Unit-Test |
| LORA-07 | The LoRa component shall wrap the Zephyr LoRa driver | Unit-Test |
| LORA-08 | The LoRa component shall configure the Zephyr LoRa driver for transmit only when sending data | Unit-Test |
| LORA-09 | The LoRa component shall provide a command to change the carrier frequency, rejecting frequencies outside the configured MIN_FREQUENCY to MAX_FREQUENCY range | Inspection |
| LORA-10 | The LoRa component shall expose enableTransmit and disableTransmit ports to control transmission | Unit-Test |
| LORA-11 | The TRANSMIT command and enableTransmit/disableTransmit ports shall enable or disable the com-status ping-pong | Unit-Test |


## Port Interfaces

| Name | Description |
|---|---|
| Svc.Com | Interface to plug the radio into the communication stack |
| Svc.BufferAllocation | Buffer allocation interface for received data |
| run | Rate group input that re-arms receive once a continuous wave ends; connect it to a rate group |
| enableTransmit | `Fw.Signal` input: enable LoRa transmission (starts com-status ping-pong) |
| disableTransmit | `Fw.Signal` input: disable LoRa transmission (stops ping-pong via `DISABLING`) |


## Configuration

| Name | Description |
|------|---|
| FREQUENCY   | Frequency of the radio transmission / receive at boot |
| MIN_FREQUENCY | Lowest frequency accepted by SET_FREQ |
| MAX_FREQUENCY | Highest frequency accepted by SET_FREQ. Defaults to FREQUENCY, as does MIN_FREQUENCY, which disables retuning |
| BANDWIDTH   | Number of parity bits sent             |
| TX_POWER    | Transmission power of the raio |
| PREAMBLE    | Preamble length in bytes |

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

## Events

| Name | Description |
|---|---|
| ConfigurationFailed | Failed to configure the LoRa radio |
| SendFailed          | Failed to send data out LoRa radio |
| AllocationFailed    | Failed to allocate buffer for received data|
| FrequencyOutOfRange | SET_FREQ frequency outside MIN_FREQUENCY to MAX_FREQUENCY |
| FrequencySet        | Carrier frequency applied by a successful SET_FREQ |
