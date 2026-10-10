// ======================================================================
// \title  LoRa.hpp
// \author lestarch
// \brief  hpp file for LoRa component implementation class
// ======================================================================

#ifndef Zephyr_LoRa_HPP
#define Zephyr_LoRa_HPP

#include "Os/Mutex.hpp"
#include "fprime-zephyr/Drv/LoRa/LoRaComponentAc.hpp"
#include <zephyr/drivers/lora.h>
#include <atomic>

namespace Zephyr {

class LoRa final : public LoRaComponentBase {
  public:
    //! Maximum payload size of the LoRa radio. This is a hardware property.
    static constexpr FwSizeType MAX_PACKET_SIZE = 252;
    // Status returned from various LoRa operations
    enum Status { NOT_READY, NOT_INITIALIZED, ERROR, SUCCESS };
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct LoRa object
    LoRa(const char* const compName  //!< The component name
    );

    //! Destroy LoRa object
    ~LoRa();

    //! LoRa receive callback
    static void receiveCallback(const struct device* dev, U8* data, U16 size, I16 rssi, I8 snr, void* user_data);

    //! Configure LoRa radio the supplied device and start it
    Status start(const struct device* lora_device, const TransmitState& transmit_enabled);

    //! Enable tx
    Status enableTx();

    //! Enable rx
    Status enableRx(bool initial=false);

  private:
    //! True when the modem reports a packet reception in progress (false on radios without status support)
    bool receiveInProgress();

    //! True while a packet is mid-air or within LoRaConfig::RX_HOLDOFF_MS of the last received packet
    bool channelBusy();

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for dataIn
    //!
    //! Data to be sent on the wire; deferred (buffer returned, FAILURE emitted) when a receive is in progress
    void dataIn_handler(FwIndexType portNum,  //!< The port number
                        Fw::Buffer& data,
                        const ComCfg::FrameContext& context) override;

    //! Handler implementation for run
    //!
    //! Re-arms receive once a continuous wave has finished and emits the recovery SUCCESS owed after a deferred
    //! transmit once the receive completes or the bound expires
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    //! Handler implementation for dataReturnIn
    //!
    //! Port receiving back ownership of buffer sent out on dataOut
    void dataReturnIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& data,
                              const ComCfg::FrameContext& context) override;

    //! Handler implementation for enableTransmit
    void enableTransmit_handler(FwIndexType portNum  //!< The port number
                                ) override;

    //! Handler implementation for disableTransmit
    void disableTransmit_handler(FwIndexType portNum  //!< The port number
                                 ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command CONTINUOUS_WAVE
    //!
    //! Start a continuous wave for the supplied number of seconds
    void CONTINUOUS_WAVE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                    U32 cmdSeq,           //!< The command sequence number
                                    U16 seconds) override;

    //! Handler implementation for command SET_FREQ
    //!
    //! Set the LoRa frequency in Hz
    void SET_FREQ_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                             U32 cmdSeq,           //!< The command sequence number
                             U32 freq_hz) override;
    
    //! Handler implementation for command TRANSMIT
    //!
    //! Start/stop transmission on the LoRa module
    void TRANSMIT_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                             U32 cmdSeq,           //!< The command sequence number
                             const TransmitState& enabled) override;

    //! Set the transmit state of the LoRa component
    //! Used by the TRANSMIT command and the enable/disable transmit port handlers
    void setTransmitState(TransmitState state);

  private:
    //! Re-arm receive if the active continuous wave has ended
    //!
    //! \return true while a continuous wave is still active
    bool updateContinuousWave();

    U8 m_send_buffer[LoRa::MAX_PACKET_SIZE];  //!< Buffer for sending data (max LoRa packet size)
    //! Process received data
    //!
    void receive(U8* data,  //!< Data to process
                 U16 size,  //!< Size of the data
                 I16 rssi,  //!< RSSI value for telemetry
                 I8 snr     //!< SNR value for telemetry
    );

    //! Pointer to the LoRa device
    const struct device* m_lora_device;
    Zephyr::TransmitState m_transmit_enabled;  //!< Transmit enabled state
    Os::Mutex m_mutex;  //!< Mutex for thread safety
    bool m_cw_active = false;  //!< Continuous wave in progress
    Fw::Time m_cw_end;         //!< Time after which receive is re-armed
    bool m_lora_ever_on = false;  //!< Latched true after transmit is first enabled

    FwSizeType m_bytes_sent = 0;     //!< Total bytes sent telemetry
    FwSizeType m_bytes_received = 0; //!< Total bytes received telemetry
    U32 m_transmits_deferred = 0;    //!< Total transmits deferred for an in-progress receive
    bool m_recovery_pending = false; //!< A deferral FAILURE was emitted and its recovery SUCCESS is owed
    U32 m_deferred_at_ms = 0;        //!< Uptime in ms when the transmit was deferred
    Os::Mutex m_recovery_mutex;      //!< Guards the deferral state shared between dataIn and run
    std::atomic<U32> m_last_rx_ms{0};  //!< Uptime in ms of the last received packet
};

}  // namespace Zephyr

#endif
