// ======================================================================
// \title  BootloaderEntry.hpp
// \brief  Platform-specific reboot into the resident bootloader
// ======================================================================

#ifndef Zephyr_BootloaderEntry_HPP
#define Zephyr_BootloaderEntry_HPP

#include <cstdint>

namespace Zephyr {
namespace Bootloader {

//! Signature of a function rebooting the board into its bootloader. Must not return.
using EntryFunction = void (*)();

//! Baud rate the platform's host tooling uses to request the bootloader (134 on Teensy, 1200 elsewhere)
extern const std::uint32_t TOUCH_BAUD;

//! Human-readable name of the bootloader entry method selected for this build
extern const char* const METHOD;

//! Reboot into the bootloader using the method selected for this build's SoC / board. Does not return.
//!
//! - RP2040 / RP2350: boot ROM USB (UF2 BOOTSEL) bootloader via `reset_usb_boot`
//! - Teensy 4.x: HalfKay bootloader via `bkpt #251`
//! - nRF52 with UF2 output: Adafruit nRF52 UF2 bootloader via GPREGRET
//! - SAMD/SAME with CONFIG_BOOTLOADER_BOSSA_{ARDUINO,ADAFRUIT_UF2}: double-tap magic at the top of SRAM
//! - Otherwise: CONFIG_RETENTION_BOOT_MODE boot mode when enabled, then a warm reboot
void enter();

}  // namespace Bootloader
}  // namespace Zephyr

#endif
