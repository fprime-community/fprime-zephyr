# Zephyr::LoRa

Wrapper for the [Zephyr LoRa driver](https://docs.zephyrproject.org/latest/connectivity/lora_lorawan/index.html). This will integrate into the communication stack.

### Typical Usage

This is used as a radio in the F Prime communication stack transmitting via the LoRa radio.

## Requirements

| Name | Description | Validation |
|---------|---|---|
| LORA-01 | The LoRa component shall interface with the Zephyr Lora driver | INSPECTION |
| LORA-02 | The LoRa component shall provide DATA_RATE and CODING_RATE as paramaters | Unit-Test |
| LORA-03 | The LoRa component shall provide FREQUENCY, BANDWITH, TRANSMIT_POWER, and PREAMBLE as configurable parameters | Unit-Test |
| LORA-04 | The LoRa component shall telemeter BytesSent and BytesRecv channels | Unit-Test |
| LORA-05 | The LoRa component shall support the Svc.Com interface | Unit-Test |
| LORA-06 | The LoRa component shall have a continuous wave command | Unit-Test |
| LORA-07 | The LoRa component shall wrap the Zephyr LoRa driver | Unit-Test |
| LORA-08 | The LoRa component shall configure the Zephyr LoRa driver for tranmit only when sending data | Unit-Test |


## Port Interfaces

| Name | Description |
|---|---|
| Svc.Com | Interface to plug the radio into the communication stack |


## Configuration

| Name | Description |
|------|---|
| FREQUENCY   | Frequency of the radio transmission / receive |
| BANDWIDTH   | Number of parity bits sent             |
| TX_POWER    | Transmission power of the raio |
| PREAMBLE    | Preamble length in bytes |

## Command

| Name | Description |
|------|---|
| CONTINUOUS_WAVE | Send continuous wave for a supplied duration |

## Parameters

| Name | Description |
|------|---|
| DATA_RATE   | Spreading factor / data rate for radio |
| CODING_RATE | Number of parity bits sent             |

## Telemetry

| Name | Description |
|---|---|
| LastRssi | RSSI value of last receive |
| LastSnr  | SNR value of last receive  |

## Events

| Name | Description |
|---|---|
| ConfigurationFailed | Failed to configure the LoRa radio |
| SendFailed          | Failed to send data out LoRa radio |
| AllocationFailed    | Failed to allocate buffer for received data|
