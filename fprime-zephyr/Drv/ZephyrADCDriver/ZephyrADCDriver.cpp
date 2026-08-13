// ======================================================================
// \title  ZephyrADCDriver.cpp
// \author thakkar
// \brief  cpp file for ZephyrADCDriver component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrADCDriver/ZephyrADCDriver.hpp"
#include <zephyr/drivers/adc.h>

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrADCDriver ::ZephyrADCDriver(const char* const compName) : ZephyrADCDriverComponentBase(compName) {}

ZephyrADCDriver ::~ZephyrADCDriver() {}

// ----------------------------------------------------------------------
// Configuration for ZephyrADCDriver
// ----------------------------------------------------------------------

void ZephyrADCDriver ::configure(const struct adc_dt_spec* adc_dev_config) {
    // Basic Asserts
    FW_ASSERT(adc_dev_config != nullptr);
    
    // Zephyr ADC driver asserts
    FW_ASSERT(adc_is_ready_dt(adc_dev_config));
    FW_ASSERT(adc_channel_setup_dt(adc_dev_config) == 0);

    struct adc_sequence sequence = {};
    sequence.buffer = this->m_adcBuffer.getData();
    sequence.buffer_size = this->m_adcBuffer.getSize();

    FW_ASSERT(adc_sequence_init_dt(adc_dev_config, &sequence) == 0);

    this->m_adc_dev_config = adc_dev_config;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ZephyrADCDriver ::enableADCSchedule_handler(FwIndexType portNum, const Zephyr::ZephyrADCPollOperation& value) {
    const U8 state = static_cast<U8>(value);
    if (this->m_adc_enabled.exchange(state) != state) {
        this->log_ACTIVITY_HI_setADCReadState(value);
    } else {
        this->log_WARNING_HI_configurationError(value);
    }
}

void ZephyrADCDriver ::poll_handler(FwIndexType portNum, U32 context) {
    if (this->m_adc_enabled.load() == static_cast<U8>(Zephyr::ZephyrADCPollOperation::ADC_POLL_ENABLED)) {
        this->publishReading();
    }
}

void ZephyrADCDriver ::readADC_handler(FwIndexType portNum) {
    this->publishReading();
}

// ----------------------------------------------------------------------
// Helper Methods for ADC Driver
// ----------------------------------------------------------------------

U16 ZephyrADCDriver::readADCValueRaw(){
    FW_ASSERT(this->m_adc_dev_config != nullptr);
    FW_ASSERT(this->m_adcBuffer.getData() != nullptr);

    struct adc_sequence sequence = {};
    sequence.buffer = this->m_adcBuffer.getData();
    sequence.buffer_size = this->m_adcBuffer.getSize();
    FW_ASSERT(adc_sequence_init_dt(this->m_adc_dev_config, &sequence) == 0);

    int status = adc_read_dt(this->m_adc_dev_config, &sequence);
    
    if (status != 0) {
        FW_ASSERT(0, status, 0);
        return 0;
    }

    U16 raw = *static_cast<U16*>(sequence.buffer);

    return raw;
}

U32 ZephyrADCDriver::readADCValueMv(U16 raw){
    int32_t mv = raw;
    FW_ASSERT(adc_raw_to_millivolts_dt(this->m_adc_dev_config, &mv) == 0);
    
    return static_cast<U32>(mv);
}

void ZephyrADCDriver::publishReading() {
    const U16 raw = this->readADCValueRaw();
    const U32 mv = this->readADCValueMv(raw);

    // Telemetry is independent of optional ADC value consumers.
    this->tlmWrite_LastMvValue(mv);
    this->tlmWrite_LastRawValue(raw);

    if (this->isConnected_adcRawValue_OutputPort(0)) {
        this->adcRawValue_out(0, raw);
    }
    if (this->isConnected_adcMvValue_OutputPort(0)) {
        this->adcMvValue_out(0, mv);
    }
}


// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void ZephyrADCDriver ::ENABLE_ADC_Schedule_cmdHandler(FwOpcodeType opCode,
                                                      U32 cmdSeq,
                                                      Zephyr::ZephyrADCPollOperation enable) {

    const U8 state = static_cast<U8>(enable);
    if (this->m_adc_enabled.exchange(state) != state) {
        this->log_ACTIVITY_HI_setADCReadState(enable);
    } else {
        this->log_WARNING_HI_configurationError(enable);
    }

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Zephyr
