# Zephyr::ZephyrTouchReset

`Zephyr::ZephyrTouchReset` implements the baud rate "touch" reset used by Arduino-style tooling. When the host opens the
monitored UART (typically the USB CDC ACM port) at the touch baud rate, the board reboots into its bootloader, so new
software can be flashed without pressing any buttons.

The component is opt-in: projects that do not instantiate it are unaffected. It has no ports. It polls the UART line
coding from the Zephyr system work queue, so the touch keeps working when F Prime threads or rate groups are starved or
hung.

## Supported Platforms

The touch baud rate and the bootloader entry method are selected at compile time from the Zephyr SoC / board
configuration (see `BootloaderEntry.cpp`):

| Platform | Selected when | Touch baud | Bootloader entry |
|---|---|---|---|
| RP2040 / RP2350 | `CONFIG_SOC_FAMILY_RPI_PICO` | 1200 | Boot ROM USB (UF2 BOOTSEL) bootloader via `reset_usb_boot()` |
| Teensy 4.0 / 4.1 / MicroMod | `CONFIG_BOARD_TEENSY40`, `CONFIG_BOARD_TEENSY41`, `CONFIG_BOARD_TEENSYMM` | 134 | HalfKay bootloader via `bkpt #251` |
| nRF52 with the Adafruit UF2 bootloader | `CONFIG_SOC_SERIES_NRF52X` and `CONFIG_BUILD_OUTPUT_UF2` | 1200 | `GPREGRET = 0x57` (UF2 reset magic), then reset |
| SAMD21 / SAMD51 with a BOSSA bootloader | `CONFIG_BOOTLOADER_BOSSA_ARDUINO` or `CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2` | 1200 | Double-tap magic in the last word of SRAM, then reset |
| Any other board | `CONFIG_RETENTION_BOOT_MODE` | 1200 | `bootmode_set(BOOT_MODE_TYPE_BOOTLOADER)`, then warm reboot |
| Any other board | (fallback) | 1200 | Warm reboot |

The selected baud rate and method are available as `Zephyr::Bootloader::TOUCH_BAUD` and `Zephyr::Bootloader::METHOD`.
Projects may override both through `configure()`, for example to use a custom bootloader.

## Usage

1. Instantiate the component in the topology. No connections are needed:

    ```
    instance touchReset: Zephyr.ZephyrTouchReset base id 0x10016000
    ```

2. After the UART is set up, configure the component with the UART device to monitor:

    ```c++
    (void)touchReset.configure(DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0)));
    ```

    `configure()` takes optional arguments: the touch baud rate, a custom entry function, and the poll period
    (default 100 ms). It returns `Fw::Success::FAILURE` and does not start monitoring when the device is not ready or the
    driver lacks line control support.

3. Enable line control in the project's `prj.conf` or board `.conf` file. `CONFIG_REBOOT` is needed by the warm reboot
   fallbacks:

    ```
    CONFIG_UART_LINE_CTRL=y
    CONFIG_REBOOT=y
    ```

    SAMD boards also need the matching bootloader variant, for example
    `CONFIG_BOOTLOADER_BOSSA=y` and `CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2=y`.

## Host

Any tool that opens the port at the touch baud rate triggers the reset, for example
`stty -F /dev/ttyACM0 1200`. The `fprime-zephyr-ci` Python package provides `fprime-zephyr-flash`, which touches the port,
waits for the bootloader, and flashes the image:

```bash
# RP2040 / RP2350 / nRF52 / SAMD UF2: copy the image to the UF2 volume once it is mounted
fprime-zephyr-flash --port /dev/ttyACM0 build-fprime-automatic-zephyr/zephyr/zephyr.uf2
# Teensy 4.x: 134 baud touch, then teensy_loader_cli
fprime-zephyr-flash --method teensy --mcu TEENSY41 --port /dev/ttyACM0 build-fprime-automatic-zephyr/zephyr/zephyr.hex
# SAMD BOSSA: 1200 baud touch, then bossac
fprime-zephyr-flash --method bossac --port /dev/ttyACM0 build-fprime-automatic-zephyr/zephyr/zephyr.bin
```

The `zephyr-ci` CI plugins accept an optional `touch-baud` key, which touches the console port before running
`flash-command`.

## Requirements

| Name | Description | Validation |
|---|---|---|
| ZephyrTouchReset-001 | The component shall reboot into the bootloader when the monitored UART baud rate equals the configured touch baud rate. | Hardware test |
| ZephyrTouchReset-002 | The component shall take no action when unconfigured or when the monitored device is not ready. | Inspection |
| ZephyrTouchReset-003 | The component shall default the touch baud rate and bootloader entry method to those of the platform being built. | Inspection, build |
| ZephyrTouchReset-004 | The component shall monitor the UART independently of F Prime threads and rate groups. | Inspection |
| ZephyrTouchReset-005 | The component shall not start monitoring when the UART driver does not support line control. | Inspection |
