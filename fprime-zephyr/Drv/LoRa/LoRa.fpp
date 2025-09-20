module Zephyr {
    enum LoRaDataRate {
        SF_6 = 6
        SF_7 = 7
        SF_8 = 8
        SF_9 = 9
        SF_10 = 10
        SF_11 = 11
        SF_12 = 12
    }
    enum LoRaCodingRate {
        CR_4_5 = 1
        CR_4_6 = 2
        CR_4_7 = 3
        CR_4_8 = 4
    }
    enum LoRaMode {
        Transmit,
        Receive
    }

    # TODO: Implement the LoRa driver functionality
    # 1. Telemetry
    # 2. Commands

    @ Wrapper for the Zephyr LoRa driver
    passive component LoRa {
        @ Import the communication interface
        import Svc.Com

        @ Import the allocation interface
        import Svc.Allocation

        @ Coding rate: number of parity bits per 4 bit
        param CODING_RATE: LoRaCodingRate default LoRaCodingRate.CR_4_5

        @ Data rate / spreading factor
        param DATA_RATE: LoRaDataRate default LoRaDataRate.SF_8

        @ No-op command
        sync command CONTINUOUS_WAVE(seconds: U16)

        @ Event to indicate configuration failure
        event ConfigurationFailed(mode: LoRaMode) severity warning high \
            format "Failed to configure LoRa into mode: {}" throttle 2

        @ Event to indicate configuration failure
        event SendFailed(status: I32) severity warning high \
            format "Failed to send LoRa message: {}" throttle 2
        
        @ Event to indicate allocation failure
        event AllocationFailed(allocation_size: FwSizeType) severity warning high \
            format "Failed to allocate buffer of: {} bytes" throttle 2

        @ Last received RSSI
        telemetry LastRssi: I16 update on change

        @ Last received SNR
        telemetry LastSnr: I8 update on change

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