// ======================================================================
// \title  ZephyrADCDriver.hpp
// \author thakkar
// \brief  hpp file for ZephyrADCDriver component implementation class
// ======================================================================

#ifndef Zephyr_ZephyrADCDriver_HPP
#define Zephyr_ZephyrADCDriver_HPP

#include <atomic>

#include "fprime-zephyr/Drv/ZephyrADCDriver/ZephyrADCDriverComponentAc.hpp"
#include "Fw/Buffer/Buffer.hpp"

struct adc_dt_spec;

namespace Zephyr {

class ZephyrADCDriver final : public ZephyrADCDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ZephyrADCDriver object
    ZephyrADCDriver(const char* const compName  //!< The component name
    );

    //! Configuration for ZephyrADCDriver 
    void configure(const struct adc_dt_spec* adc_dev_config);

    //! Destroy ZephyrADCDriver object
    ~ZephyrADCDriver();

  private:
    // ----------------------------------------------------------------------
    // Helper Methods for ADC Driver
    // ----------------------------------------------------------------------
    
    //! Read ADC Raw Value
    U16 readADCValueRaw();

    //! Read ADC Value in millivolts
    U32 readADCValueMv(U16 raw);

    //! Read one ADC sample and publish it to telemetry and connected output ports
    void publishReading();
    
    // ----------------------------------------------------------------------
    // Configuration Parameters for ZephyrADCDriver
    // ----------------------------------------------------------------------

    //!< Zephyr ADC device specification
    const struct adc_dt_spec* m_adc_dev_config = nullptr; 

    //!< ADC sample buffer
    const U16 m_dataBuffer[1] = {0};
    Fw::Buffer m_adcBuffer{reinterpret_cast<U8*>(const_cast<U16*>(m_dataBuffer)), sizeof(m_dataBuffer)};

    //!< Flag to indicate if ADC is continuous polling enabled or disabled
    std::atomic<U8> m_adc_enabled{static_cast<U8>(Zephyr::ZephyrADCPollOperation::ADC_POLL_DISABLED)};

    
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for enableADCSchedule
    //!
    //! Port to enable or disable the ADC
    void enableADCSchedule_handler(FwIndexType portNum,  //!< The port number
                                   const Zephyr::ZephyrADCPollOperation& value) override;

    //! Handler implementation for poll
    void poll_handler(FwIndexType portNum,  //!< The port number
                      U32 context           //!< The call order
                      ) override;

    //! Handler implementation for readADC
    //!
    //! Port to read the ADC value
    void readADC_handler(FwIndexType portNum  //!< The port number
                         ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command ENABLE_ADC_Schedule
    void ENABLE_ADC_Schedule_cmdHandler(
        FwOpcodeType opCode,                 //!< The opcode
        U32 cmdSeq,                          //!< The command sequence number
        Zephyr::ZephyrADCPollOperation enable  //!< Indicates whether the ADC is enabled or disabled
        ) override;   
};

}  // namespace Zephyr

#endif
