module Zephyr {
    
    # @ Zephyr ADC Driver State
    enum ZephyrADCPollOperation: U8 {
        ADC_POLL_DISABLED = 0,
        ADC_POLL_ENABLED = 1
    }

    # @ Port Types
    port ADCMvValue(value: U32)
    port ADCRawValue(value: U16)
    port ZephyrADCPollOperationPort(value: ZephyrADCPollOperation)

    @ Zephyr ADC Driver
    passive component ZephyrADCDriver {

        sync command ENABLE_ADC_Schedule(
            enable: ZephyrADCPollOperation @< Indicates whether the ADC is enabled or disabled
        )
        
        # @ Last value read from the ADC in millivolts
        telemetry LastMvValue: U32 update always

        # @ Last raw value read from the ADC
        telemetry LastRawValue: U16 update always
        
        # @ Example event
        # event ExampleStateEvent(example_state: Fw.On) severity activity high id 0 format "State set to {}"
        event setADCReadState($enable: ZephyrADCPollOperation) \
            severity activity high \
            format "Enable ADC set set to state: {}"
        
        event configurationError($error: ZephyrADCPollOperation) \
            severity warning high \
            format "ADC configuration error; State Not Enabled; Curr State: {}"
        
        event connectionError() \
            severity warning high \
            format "ADC output port is not connected"

        # @ Schedule port to be read on a given rate group
        sync input port poll: Svc.Sched

        guarded input port enableADCSchedule: ZephyrADCPollOperationPort @< Port to enable or disable the ADC

        guarded input port readADC: Fw.Signal @< Signal the driver to read one ADC sample

        output port adcMvValue: ADCMvValue @< Port to output the ADC value in millivolts
        
        output port adcRawValue: ADCRawValue @< Port to output the ADC raw value

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

    }
}
