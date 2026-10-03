#ifndef LORA_CFG_HPP
#define LORA_CFG_HPP
#include <zephyr/drivers/lora.h>
#include <Fw/FPrimeBasicTypes.hpp>
namespace LoRaConfig {
const U32 FREQUENCY = 915000000;               //!< LoRa frequency in Hz at boot (e.g. 437400000)
const U32 MIN_FREQUENCY = FREQUENCY;           //!< Lowest SET_FREQ frequency in Hz (MIN == MAX disables)
const U32 MAX_FREQUENCY = FREQUENCY;           //!< Highest SET_FREQ frequency in Hz
lora_signal_bandwidth BANDWIDTH = BW_125_KHZ;  //!< LoRa bandwidth
const I8 TX_POWER = 14;                        //!< LoRa transmission power in dBm
const U16 PREAMBLE_LENGTH = 8;                 //!< LoRa preamble length
U8 HEADER[] = {0, 0, 0, 0};                    //!< LoRa header (not used)
}  // namespace LoRaConfig
#endif  // LORA_CFG_HPP
