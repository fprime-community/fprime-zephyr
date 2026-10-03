# Zephyr::ZephyrTouchReset

`Zephyr::ZephyrTouchReset` implements the "1200 baud touch" reset used by Arduino-style tooling: when the host opens the
monitored UART (typically the USB CDC ACM console) at the touch baud rate, the board reboots into its bootloader so that
new software can be flashed without pressing any buttons.

The component is opt-in. Projects that do not instantiate it are unaffected.

## Usage

1. Instantiate the component and call its `run` port from a rate group (e.g. 10Hz).
2. Configure it with the UART device to monitor:

    ```c++
    touchReset.configure(DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0)));
    ```

3. Enable the Zephyr options below in the project's `prj.conf` or board `.conf` file.

    ```
    CONFIG_UART_LINE_CTRL=y
    CONFIG_REBOOT=y
    ```

4. Enable Zephyr's boot mode retention so that the reboot lands in the bootloader rather than the application. On
   RP2040/RP2350 this is the stock `rp2-boot-mode-retention` snippet, which selects `CONFIG_RPI_PICO_ROM_BOOTLOADER` and
   reboots into the ROM UF2 (BOOTSEL) bootloader. It may be applied with `-DSNIPPET=rp2-boot-mode-retention` or by
   including its contents in the project's board overlay and `.conf` files:

    ```
    #include <vendor/raspberrypi/rp2040-boot-mode-retention.dtsi>
    ```

    ```
    CONFIG_RETAINED_MEM=y
    CONFIG_RETENTION=y
    CONFIG_RETENTION_BOOT_MODE=y
    ```

   Without `CONFIG_RETENTION_BOOT_MODE` the component performs a plain warm reboot.

## Host

Any tool that opens the port at the touch baud rate triggers the reset, for example:

```bash
stty -F /dev/ttyACM0 1200
```

## Requirements

| Name | Description | Validation |
|---|---|---|
| ZephyrTouchReset-001 | The component shall reboot into the bootloader when the monitored UART baud rate equals the configured touch baud rate. | Hardware test |
| ZephyrTouchReset-002 | The component shall take no action when unconfigured or when the monitored device is not ready. | Inspection |
