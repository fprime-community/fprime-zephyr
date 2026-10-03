// ======================================================================
// \title  BootloaderEntry.cpp
// \brief  Platform-specific reboot into the resident bootloader
// ======================================================================

#include "fprime-zephyr/Svc/ZephyrTouchReset/BootloaderEntry.hpp"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

// Select exactly one platform; the first match wins
#if defined(CONFIG_SOC_FAMILY_RPI_PICO)
#define ZEPHYR_BOOTLOADER_RPI_PICO
#include <pico/bootrom.h>
#elif defined(CONFIG_BOARD_TEENSY40) || defined(CONFIG_BOARD_TEENSY41) || defined(CONFIG_BOARD_TEENSYMM)
#define ZEPHYR_BOOTLOADER_TEENSY
#elif defined(CONFIG_SOC_SERIES_NRF52X) && defined(CONFIG_BUILD_OUTPUT_UF2)
#define ZEPHYR_BOOTLOADER_NRF52_UF2
#include <soc.h>
#elif defined(CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2) || defined(CONFIG_BOOTLOADER_BOSSA_ARDUINO)
#define ZEPHYR_BOOTLOADER_SAM0_BOSSA
#include <soc.h>
#include <zephyr/devicetree.h>
#elif defined(CONFIG_RETENTION_BOOT_MODE)
#define ZEPHYR_BOOTLOADER_RETENTION
#include <zephyr/retention/bootmode.h>
#else
#define ZEPHYR_BOOTLOADER_REBOOT
#endif

namespace Zephyr {
namespace Bootloader {

#if defined(ZEPHYR_BOOTLOADER_RPI_PICO)

const std::uint32_t TOUCH_BAUD = 1200;
const char* const METHOD = "RP2 boot ROM (UF2 BOOTSEL)";

void enter() {
    reset_usb_boot(0, 0);
}

#elif defined(ZEPHYR_BOOTLOADER_TEENSY)

// Teensyduino and teensy_loader tooling use 134 baud to request the bootloader
const std::uint32_t TOUCH_BAUD = 134;
const char* const METHOD = "Teensy HalfKay (bkpt #251)";

void enter() {
    // The bootloader chip monitors the debug port and takes over on this breakpoint
    __asm__ volatile("bkpt #251");
    sys_reboot(SYS_REBOOT_COLD);
}

#elif defined(ZEPHYR_BOOTLOADER_NRF52_UF2)

const std::uint32_t TOUCH_BAUD = 1200;
const char* const METHOD = "Adafruit nRF52 UF2 (GPREGRET)";

void enter() {
    // DFU_MAGIC_UF2_RESET from the Adafruit nRF52 bootloader
    constexpr std::uint32_t UF2_RESET_MAGIC = 0x57;
    (void)irq_lock();
    NRF_POWER->GPREGRET = UF2_RESET_MAGIC;
    NVIC_SystemReset();
}

#elif defined(ZEPHYR_BOOTLOADER_SAM0_BOSSA)

const std::uint32_t TOUCH_BAUD = 1200;
const char* const METHOD = "SAM0 BOSSA / UF2 (double-tap magic)";

void enter() {
#if defined(CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2)
    constexpr std::uint32_t DOUBLE_TAP_MAGIC = 0xf01669efU;
#else
    constexpr std::uint32_t DOUBLE_TAP_MAGIC = 0x07738135U;
#endif
    // The bootloader stays resident when the last word of SRAM holds the magic on reset
    volatile std::uint32_t* const top =
        reinterpret_cast<volatile std::uint32_t*>(DT_REG_ADDR(DT_NODELABEL(sram0)) + DT_REG_SIZE(DT_NODELABEL(sram0)));
    (void)irq_lock();
    top[-1] = DOUBLE_TAP_MAGIC;
    NVIC_SystemReset();
}

#elif defined(ZEPHYR_BOOTLOADER_RETENTION)

const std::uint32_t TOUCH_BAUD = 1200;
const char* const METHOD = "retention boot mode";

void enter() {
    const int status = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
    if (status != 0) {
        printk("[TouchReset] bootmode_set failed (%d), not rebooting\n", status);
        return;
    }
    sys_reboot(SYS_REBOOT_WARM);
}

#else

const std::uint32_t TOUCH_BAUD = 1200;
const char* const METHOD = "warm reboot (no bootloader entry method for this platform)";

void enter() {
    sys_reboot(SYS_REBOOT_WARM);
}

#endif

}  // namespace Bootloader
}  // namespace Zephyr
