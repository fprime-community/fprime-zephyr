# Zephyr::ZephyrTouchReset

`Zephyr::ZephyrTouchReset` implements the baud rate "touch" reset used by Arduino-style tooling. When the host switches
the monitored UART (typically the USB CDC ACM port) to the touch baud rate, for example by opening it at that rate, the
board reboots into its bootloader, so new software can be flashed without pressing any buttons.

The component is opt-in: projects that do not instantiate it are unaffected. It has no ports. It polls the UART line
coding from the Zephyr system work queue, so the touch keeps working when F Prime threads or rate groups are starved or
hung.

> [!WARNING]
> Any host process that can open the monitored port can reboot the board into its bootloader and, because these
> bootloaders accept unsigned images, reflash it. The bootloader entry is not reported through F Prime events or
> telemetry. Use this component on development builds, and do not monitor a UART that carries a fielded ground link.

## Design

`ZephyrTouchReset` is a kernel-context helper that sits outside the F Prime execution model: it is an F Prime component
only so that it is instantiated and configured with the rest of the topology. It deliberately has no `Svc.Sched` port.
Its independence from F Prime (ZephyrTouchReset-004) relies on the Zephyr system work queue running at a cooperative
priority (`CONFIG_SYSTEM_WORKQUEUE_PRIORITY`, default -1), above the preemptible priorities used by F Prime tasks, and on
no other work item blocking that queue.

The bootloader is entered when the baud rate *changes* to the touch baud rate and stays there for two consecutive polls:
a reading of the touch baud rate takes effect only after a different baud rate has been observed since `configure()`,
and only when the next poll reads the touch baud rate again. A touch therefore takes up to two poll periods (200 ms by
default) to take effect. `configure()` asserts that the poll period is between 20 ms and 1000 ms. At 1000 ms a touch
takes at most 2 s, within the 5 s the CI `touch-baud` key waits for the port to go away.

The second reading is needed because Linux saves a port's baud rate, and when a tool opens the port it first re-sends
the saved rate (for example 1200 after a touch) before the tool applies its own rate. That replay normally lasts far
less than a poll period, but it is ignored only when the opening tool applies its own rate within one poll period P. A
tool that takes longer (for example a stalled process on a loaded host) enters the bootloader with probability (delay -
P) / P, and always once the delay reaches 2P. Keep the 100 ms default unless the hosts that open the port are known to
be fast. The edge trigger only ignores a touch baud rate the device already reports at its first poll: the USB CDC ACM
line coding starts at 115200 and the Linux `cdc-acm` driver sets 9600 when the device enumerates, so the component is
normally armed before any host opens the port. So:

- Do not choose a touch baud rate that ground, CI, or terminal tools use to open the port.
- After any touch (including `fprime-zephyr-flash`, the CI `touch-baud` key, and `stty`), Linux keeps the touch baud
  rate as the port's saved rate. Reset the host port, for example `stty -F /dev/ttyACM0 115200`, before tools that do
  not set a baud rate (such as `cat`) open it again.

Like the RP2040 (pico-sdk) and Teensy conventions, the trigger is the baud rate alone; DTR is not checked, so a touch
that leaves DTR asserted (e.g. `stty -hupcl`) still works. If the entry function returns, for example because
`bootmode_set()` fails, the component waits for another change to the touch baud rate before retrying. The bootloader
message is printed with `printk` to keep the system work queue stack use small.

## Supported Platforms

The touch baud rate and the bootloader entry method are selected at compile time from the Zephyr SoC / board
configuration (see `BootloaderEntry.cpp`):

| Platform | Selected when | Touch baud | Bootloader entry |
|---|---|---|---|
| RP2040 / RP2350 | `CONFIG_SOC_FAMILY_RPI_PICO` | 1200 | Boot ROM USB (UF2 BOOTSEL) bootloader via `reset_usb_boot()` |
| Teensy 4.0 / 4.1 / MicroMod | `CONFIG_BOARD_TEENSY40`, `CONFIG_BOARD_TEENSY41`, `CONFIG_BOARD_TEENSYMM` | 134 | HalfKay bootloader via `bkpt #251` |
| nRF52 with the Adafruit UF2 bootloader | `CONFIG_SOC_SERIES_NRF52X` and `CONFIG_BUILD_OUTPUT_UF2` | 1200 | `GPREGRET = 0x57` (UF2 reset magic), then reset |
| SAMD21 / SAMD51 with a BOSSA bootloader | `CONFIG_BOOTLOADER_BOSSA_ARDUINO` or `CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2` | 1200 | Double-tap magic in the last word of SRAM, then reset |
| Any other board | `CONFIG_RETENTION_BOOT_MODE` | 1200 | `bootmode_set(BOOT_MODE_TYPE_BOOTLOADER)`, then warm reboot (no reboot if `bootmode_set()` fails) |
| Any other board | (fallback) | 1200 | Warm reboot |

The selected baud rate and method are available as `Zephyr::Bootloader::TOUCH_BAUD` and `Zephyr::Bootloader::METHOD`.
Projects may override the baud rate and the entry function through `configure()`, for example to use a custom
bootloader. The log message still names the built-in `METHOD`.

## Usage

1. Instantiate the component in the topology. No connections are needed:

    ```
    instance touchReset: Zephyr.ZephyrTouchReset base id 0x10016000
    ```

2. After the UART is set up, configure the component with the UART device to monitor:

    ```c++
    if (touchReset.configure(DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0))) != Fw::Success::SUCCESS) {
        Fw::Logger::log("Touch reset unavailable\n");
    }
    ```

    `configure()` takes optional arguments: the touch baud rate, a custom entry function, and the poll period
    (default 100 ms, asserted to be within 20 to 1000 ms; see Design). It returns `Fw::Success::FAILURE` and does not monitor when the device is not ready or
    `uart_line_ctrl_get(UART_LINE_CTRL_BAUD_RATE)` fails, for example because `CONFIG_UART_LINE_CTRL` is disabled or the
    driver cannot report its baud rate. Calling `configure()` again stops any earlier monitoring first.

3. Enable line control in the project's `prj.conf` or board `.conf` file. `CONFIG_REBOOT` is needed by the Teensy,
   retention boot mode, and warm reboot entry methods:

    ```
    CONFIG_UART_LINE_CTRL=y
    CONFIG_REBOOT=y
    ```

    SAMD boards also need the matching bootloader variant, for example
    `CONFIG_BOOTLOADER_BOSSA=y` and `CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2=y`. With the legacy USB device stack, Zephyr's
    own `soc/atmel/sam0/common/bossa.c` can also perform the 1200 baud reset, but only when
    `CONFIG_BOOTLOADER_BOSSA_DEVICE_NAME` (default `"CDC_ACM_0"`) matches the CDC ACM device name. A node such as
    `cdc_acm_uart0` without a `label` is named `cdc_acm_uart0`, so either set that option or use this component, which
    is required with the `device_next` USB stack. Unlike this component, `bossa.c` resets on the first 1200 baud line
    coding, including the saved rate Linux replays when a port is opened after a touch (see Design). Prefer this
    component, and with the legacy stack keep the two from both monitoring the same port (e.g.
    `CONFIG_CDC_ACM_DTE_RATE_CALLBACK_SUPPORT=n`).

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

When `--volume` is omitted, the UF2 method only accepts a volume that appears after the touch, and fails when several
appear; without `--port` (board already in its bootloader) it accepts an already-mounted volume. When the bootloader reuses the
touched port, the bossac method fails if that port does not go away. `--method teensy` runs [`teensy_loader_cli`](https://github.com/PaulStoffregen/teensy_loader_cli) and
`--method bossac` runs [`bossac`](https://github.com/shumatech/BOSSA): the first binary of that name on `PATH` is used,
so install them from these upstreams or the OS package manager.

The `zephyr-ci` CI plugins accept an optional `touch-baud` key, which touches the console port and waits up to 5 s for it
to disappear before running `flash-command`. After the timeout, a warning is logged and flashing proceeds.

## Requirements

| Name | Description | Validation |
|---|---|---|
| ZephyrTouchReset-001 | The component shall reboot into the bootloader when the monitored UART baud rate changes to the configured touch baud rate (see ZephyrTouchReset-006). | Hardware test |
| ZephyrTouchReset-002 | The component shall take no action when unconfigured or when the monitored device is not ready. | Inspection |
| ZephyrTouchReset-003 | The component shall default the touch baud rate and bootloader entry method to those of the platform being built. | Inspection, build |
| ZephyrTouchReset-004 | The component shall monitor the UART independently of F Prime threads and rate groups. | Inspection |
| ZephyrTouchReset-005 | The component shall not monitor the UART when its baud rate cannot be read through `uart_line_ctrl_get`. | Inspection |
| ZephyrTouchReset-006 | The component shall enter the bootloader only after observing a baud rate other than the touch baud rate since configuration, and only after reading the touch baud rate on two consecutive polls. | Inspection, hardware test |

## Verification

The component has no unit tests: fprime-zephyr has no native unit test build, and the component's behavior depends on
the Zephyr work queue and USB CDC ACM driver. Requirements are verified by inspection, by target builds of each
platform branch, and by hardware tests. The host tool is covered by `ci/test/test_touch.py` and the CI plugin's
`touch-baud` preload by `ci/test/test_zephyr_ci.py`.
