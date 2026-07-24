module Zephyr {
    enum LoRaDataRate : U8{
        # EXCLUDED: DOES not work. SF_6 = 6
        SF_7 = 7
        SF_8 = 8
        SF_9 = 9
        SF_10 = 10
        # EXCLUDED: KILLS radio SF_11 = 11
        # EXCLUDED: KILLS radio SF_12 = 12
    }
    enum LoRaCodingRate : U8 {
        CR_4_5 = 1
        CR_4_6 = 2
        CR_4_7 = 3
        CR_4_8 = 4
    }
    @ Values match zephyr's enum lora_signal_bandwidth (kHz), which LoRa.cpp casts directly into
    enum LoRaBandwidth : U16 {
        BW_125_KHZ = 125
        BW_250_KHZ = 250
        BW_500_KHZ = 500
    }
    enum LoRaMode : U8 {
        Transmit,
        Receive
    }
    enum TransmitState : U8 {
        ENABLED,
        DISABLED,
        DISABLING,
    }

    @ Wrapper for the Zephyr LoRa driver
    passive component LoRa {
        @ Import the communication interface
        import Svc.Com

        @ Import the allocation interface
        import Svc.BufferAllocation

        @ Coding rate: number of parity bits per 4 bit
        param CODING_RATE: LoRaCodingRate default LoRaCodingRate.CR_4_5

        @ Data rate / spreading factor
        param DATA_RATE: LoRaDataRate default LoRaDataRate.SF_8

        @ Bandwidth for transmission
        param BANDWIDTH_TX: LoRaBandwidth default LoRaBandwidth.BW_125_KHZ

        @ Bandwidth for reception
        param BANDWIDTH_RX: LoRaBandwidth default LoRaBandwidth.BW_125_KHZ

        @ Continuous wave transmission
        sync command CONTINUOUS_WAVE(seconds: U16)

        @ Rate group port that re-arms receive after a continuous wave
        sync input port run: Svc.Sched

        @ Set the LoRa carrier frequency in Hz. Rejected outside LoRaConfig::MIN_FREQUENCY..MAX_FREQUENCY;
        @ not persisted, so the radio returns to LoRaConfig::FREQUENCY on reboot
        sync command SET_FREQ(freq_hz: U32)

        @ Start/stop transmission on the LoRa module
        sync command TRANSMIT(enabled: TransmitState)

        @ Event to indicate configuration failure
        event ConfigurationFailed(mode: LoRaMode) severity warning high \
            format "Failed to configure LoRa into mode: {}" throttle 2

        @ Event to indicate configuration failure
        event SendFailed(status: I32) severity warning high \
            format "Failed to send LoRa message: {}" throttle 2
        
        @ Event to indicate a SET_FREQ frequency outside the configured range
        event FrequencyOutOfRange(freq_hz: U32, min_hz: U32, max_hz: U32) severity warning low \
            format "Frequency {} Hz outside allowed range [{}, {}] Hz"

        @ Event to indicate the carrier frequency was changed
        event FrequencySet(freq_hz: U32) severity activity high \
            format "LoRa frequency set to {} Hz"

        @ Event to indicate allocation failure
        event AllocationFailed(allocation_size: FwSizeType) severity warning high \
            format "Failed to allocate buffer of: {} bytes" throttle 2

        @ Bytes received
        telemetry BytesReceived: FwSizeType update on change

        @ Bytes received
        telemetry BytesSent: FwSizeType update on change

        @ Last received RSSI
        telemetry LastRssi: I16 update on change

        @ Last received SNR
        telemetry LastSnr: I8 update on change

        @ Enable LoRa transmission
        sync input port enableTransmit: Fw.Signal

        @ Disable LoRa transmission
        sync input port disableTransmit: Fw.Signal

        @ Emitted once when LoRa transmit is first enabled
        output port loraEverOn: Fw.Signal

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}
