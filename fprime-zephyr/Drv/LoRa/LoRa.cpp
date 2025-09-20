// ======================================================================
// \title  LoRa.cpp
// \author lestarch
// \brief  cpp file for LoRa component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/LoRa/LoRa.hpp"
#include "zephyr-config/LoRaCfg.hpp"

namespace Zephyr {

// Base configuration for the LoRa modem
struct lora_modem_config BASE_CONFIG = {
    .frequency = LoRaConfig::FREQUENCY,
    .bandwidth = BW_125_KHZ,
    .datarate = SF_8,
    .coding_rate = CR_4_5,
    .preamble_len = LoRaConfig::PREAMBLE_LENGTH,
    .tx_power = LoRaConfig::TX_POWER,
    .tx = false,
    .iq_inverted = false,
    .public_network = false,
};

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

void LoRa::receiveCallback(const struct device* dev, U8* data, U16 size, I16 rssi, I8 snr, void* user_data) {
    Zephyr::LoRa* lora_component = static_cast<Zephyr::LoRa*>(user_data);
    FW_ASSERT(lora_component != nullptr);
    lora_component->receive(data, size, rssi, snr);
}

LoRa ::LoRa(const char* const compName) : LoRaComponentBase(compName) {}

LoRa ::~LoRa() {}

LoRa::Status LoRa ::start(const struct device* lora_device) {
    if (!device_is_ready(lora_device)) {
        return NOT_READY;
    }
    this->m_lora_device = lora_device;

    LoRa::Status config_status = this->enableRx();
    if (config_status != Status::SUCCESS) {
        this->log_WARNING_HI_ConfigurationFailed(LoRaMode::Receive);
        return LoRa::Status::ERROR;
    }
    Fw::Success status = Fw::Success::SUCCESS;
    this->comStatusOut_out(0, status);
    return Status::SUCCESS;
}

LoRa::Status LoRa ::enableTx() {
    Fw::ParamValid isValid = Fw::ParamValid::INVALID;
    const LoRaDataRate data_rate = this->paramGet_DATA_RATE(isValid);
    FW_ASSERT(isValid != Fw::ParamValid::VALID, static_cast<FwAssertArgType>(isValid));
    const LoRaCodingRate coding_rate = this->paramGet_CODING_RATE(isValid);
    FW_ASSERT(isValid != Fw::ParamValid::VALID, static_cast<FwAssertArgType>(isValid));

    // Disable async receive while in TX mode
    int status = lora_recv_async(this->m_lora_device, NULL, NULL);
    if (status == 0) {
        // Update BASE_CONFIG in-place to save on stack space
        BASE_CONFIG.tx = true;
        BASE_CONFIG.datarate = static_cast<enum lora_datarate>(data_rate.e);
        BASE_CONFIG.coding_rate = static_cast<enum lora_coding_rate>(coding_rate.e);
        status = lora_config(this->m_lora_device, &BASE_CONFIG);
    }
    return (status < 0) ? Status::ERROR : Status::SUCCESS;
}

LoRa::Status LoRa ::enableRx() {
    Fw::ParamValid isValid = Fw::ParamValid::INVALID;
    const LoRaDataRate data_rate = this->paramGet_DATA_RATE(isValid);
    FW_ASSERT(isValid != Fw::ParamValid::INVALID, static_cast<FwAssertArgType>(isValid));
    const LoRaCodingRate coding_rate = this->paramGet_CODING_RATE(isValid);
    FW_ASSERT(isValid != Fw::ParamValid::INVALID, static_cast<FwAssertArgType>(isValid));

    // Update BASE_CONFIG in-place to save on stack space
    BASE_CONFIG.tx = false;
    BASE_CONFIG.datarate = static_cast<enum lora_datarate>(data_rate.e);
    BASE_CONFIG.coding_rate = static_cast<enum lora_coding_rate>(coding_rate.e);
    int status = lora_config(this->m_lora_device, &BASE_CONFIG);
    if (status == 0) {
        status = lora_recv_async(this->m_lora_device, LoRa::receiveCallback, this);
    }
    return (status < 0) ? Status::ERROR : Status::SUCCESS;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void LoRa ::dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    Os::ScopeLock lock(this->m_mutex);
    FW_ASSERT(data.getSize() + sizeof(LoRaConfig::HEADER) <= 252, data.getSize());
    FW_ASSERT(this->m_lora_device != nullptr);
    FW_ASSERT(device_is_ready(this->m_lora_device));
    Fw::Success returnStatus = Fw::Success::SUCCESS;
    Status status = this->enableTx();
    if (status == Status::SUCCESS) {
        ::memcpy(this->m_send_buffer, LoRaConfig::HEADER, sizeof(LoRaConfig::HEADER));
        ::memcpy(this->m_send_buffer + sizeof(LoRaConfig::HEADER), data.getData(), data.getSize());
        int send_status =
            lora_send(this->m_lora_device, this->m_send_buffer, sizeof(LoRaConfig::HEADER) + data.getSize());
        if (send_status != 0) {
            this->log_WARNING_HI_SendFailed(static_cast<I32>(send_status));
            returnStatus = Fw::Success::FAILURE;
        } else {
            returnStatus = Fw::Success::SUCCESS;
            this->log_WARNING_HI_ConfigurationFailed_ThrottleClear();
            this->log_WARNING_HI_SendFailed_ThrottleClear();
        }
    } else {
        this->log_WARNING_HI_ConfigurationFailed(LoRaMode::Transmit);
        returnStatus = Fw::Success::FAILURE;
    }
    // Enable RX mode after all TXes
    status = this->enableRx();
    if (status != Status::SUCCESS) {
        this->log_WARNING_HI_ConfigurationFailed(LoRaMode::Receive);
    }
    this->dataReturnOut_out(0, data, context);
    this->comStatusOut_out(0, returnStatus);
}

void LoRa ::dataReturnIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    this->deallocate_out(0, data);
}

void LoRa ::receive(U8* data, U16 size, I16 rssi, I8 snr) {
    const FwSizeType payload_size = static_cast<FwSizeType>(size - sizeof(LoRaConfig::HEADER));
    Fw::Buffer buffer = this->allocate_out(0, payload_size);
    if (buffer.isValid()) {
        ::memcpy(buffer.getData(), data + sizeof(LoRaConfig::HEADER), payload_size);
        ComCfg::FrameContext frameContext;
        this->dataOut_out(0, buffer, frameContext);
    } else {
        this->log_WARNING_HI_AllocationFailed(size);
    }
    this->tlmWrite_LastRssi(rssi);
    this->tlmWrite_LastSnr(snr);
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void LoRa ::CONTINUOUS_WAVE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U16 seconds) {
    Status status = this->enableTx();
    if (status == Status::SUCCESS) {
        lora_test_cw(this->m_lora_device, LoRaConfig::FREQUENCY, LoRaConfig::TX_POWER, seconds);
        status = this->enableRx();
    }
    this->cmdResponse_out(opCode, cmdSeq,
                          (status == Status::SUCCESS) ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
}

}  // namespace Zephyr
