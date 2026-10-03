""" fprime_zephyr.touch: reboot boards into their bootloader with a baud rate "touch" and flash them

Many USB boards reboot into their bootloader when the host opens their USB CDC ACM port at a special "touch" baud rate:
1200 for RP2040/RP2350, SAMD (BOSSA/UF2), and nRF52 (UF2) and 134 for Teensy. F Prime Zephyr deployments get this
behavior from the Zephyr.ZephyrTouchReset component. This module provides the host side: touch the port, wait for the
bootloader to appear, and flash the image.
"""
import argparse
import logging
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Iterable, List, Optional

import serial

LOGGER = logging.getLogger(__name__)

DEFAULT_TOUCH_BAUD = 1200
TEENSY_TOUCH_BAUD = 134

# UF2 bootloaders expose a mass-storage volume containing this file
UF2_INFO_FILE = "INFO_UF2.TXT"
UF2_SEARCH_ROOTS = ["/media", "/run/media", "/Volumes", "/mnt"]

METHODS = {
    "uf2": DEFAULT_TOUCH_BAUD,
    "bossac": DEFAULT_TOUCH_BAUD,
    "teensy": TEENSY_TOUCH_BAUD,
}


class TouchFlashError(Exception):
    """ Raised when the board cannot be put into its bootloader or flashed """


def touch(port: str, baud: int = DEFAULT_TOUCH_BAUD, settle: float = 0.2):
    """ Open and close a serial port at the touch baud rate, requesting that the board enter its bootloader

    Opening the port sends the baud rate to the device (USB CDC SET_LINE_CODING). The board reboots on its own, so the
    port usually disappears shortly after this call.

    Args:
        port: serial port of the running board (e.g. /dev/ttyACM0)
        baud: touch baud rate
        settle: seconds to hold the port open so the device can observe the line coding
    """
    LOGGER.info("Touching %s at %d baud", port, baud)
    try:
        handle = serial.Serial(port, baudrate=baud)
    except serial.SerialException as exception:
        raise TouchFlashError(f"Failed to open {port} for touch: {exception}") from exception
    try:
        time.sleep(settle)
        handle.dtr = False
    except (serial.SerialException, OSError):
        # The board may already be rebooting and gone
        pass
    finally:
        try:
            handle.close()
        except (serial.SerialException, OSError):
            pass


def _subdirectories(path: Path) -> List[Path]:
    """ List the real (non-symlink) subdirectories of path, skipping any that cannot be read """
    try:
        children = list(path.iterdir())
    except OSError:
        return []
    return [child for child in children if not child.is_symlink() and child.is_dir()]


def _is_uf2_volume(path: Path) -> bool:
    """ Check whether path holds a UF2 bootloader INFO file """
    try:
        return (path / UF2_INFO_FILE).is_file()
    except OSError:
        return False


def find_uf2_volumes(roots: Iterable[str] = UF2_SEARCH_ROOTS) -> List[Path]:
    """ Find mounted UF2 bootloader volumes (directories containing INFO_UF2.TXT) up to two levels below roots

    Symbolic links and unreadable directories are skipped.
    """
    volumes = []
    for root in roots:
        root_path = Path(root)
        if not root_path.is_dir():
            continue
        children = _subdirectories(root_path)
        candidates = [root_path] + children + [grandchild for child in children for grandchild in _subdirectories(child)]
        volumes.extend(candidate for candidate in candidates if _is_uf2_volume(candidate))
    return volumes


def wait_for(predicate, timeout: float, interval: float = 0.2):
    """ Poll predicate until it returns a truthy value or timeout seconds elapse, returning its last result """
    deadline = time.monotonic() + timeout
    result = predicate()
    while not result and time.monotonic() < deadline:
        time.sleep(interval)
        result = predicate()
    return result


def new_uf2_volumes(existing: Iterable[Path]) -> List[Path]:
    """ Find UF2 volumes that are not in existing """
    existing = set(existing)
    return [volume for volume in find_uf2_volumes() if volume not in existing]


def copy_image(image: Path, volume: Path) -> Path:
    """ Copy image into volume without following a symbolic link at the destination """
    destination = volume / image.name
    descriptor = os.open(destination, os.O_WRONLY | os.O_CREAT | os.O_TRUNC | getattr(os, "O_NOFOLLOW", 0), 0o644)
    with open(image, "rb") as source, os.fdopen(descriptor, "wb") as target:
        shutil.copyfileobj(source, target)
    return destination


def flash_uf2(image: Path, volume: Optional[Path], timeout: float, existing: Iterable[Path] = ()) -> Path:
    """ Copy a UF2 image onto the UF2 bootloader volume, waiting for the volume to be mounted

    Args:
        image: UF2 image to flash
        volume: bootloader volume mount point, or None to search the usual mount locations
        timeout: seconds to wait for the volume
        existing: volumes mounted before the touch, which are ignored when searching
    Returns:
        volume the image was copied to
    """
    if image.suffix.lower() != ".uf2":
        raise TouchFlashError(f"UF2 flashing requires a .uf2 image, got {image}")
    if volume is not None:
        found = volume if wait_for(lambda: _is_uf2_volume(volume), timeout) else None
    else:
        volumes = wait_for(lambda: new_uf2_volumes(existing), timeout)
        if len(volumes) > 1:
            raise TouchFlashError(f"Several UF2 volumes appeared ({', '.join(map(str, volumes))}), select one with --volume")
        found = volumes[0] if volumes else None
    if found is None:
        raise TouchFlashError(f"No new UF2 bootloader volume appeared within {timeout}s (is it mounted?)")
    LOGGER.info("Copying %s to %s", image, found)
    copy_image(image, found)
    os.sync()
    return found


def run_tool(command: List[str], timeout: float):
    """ Run an external flashing tool, raising TouchFlashError on failure """
    LOGGER.info("Running: %s", " ".join(command))
    try:
        subprocess.run(command, check=True, timeout=timeout)
    except FileNotFoundError as exception:
        raise TouchFlashError(f"{command[0]} not found on PATH") from exception
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exception:
        raise TouchFlashError(f"{command[0]} failed: {exception}") from exception


def flash_teensy(image: Path, mcu: str, timeout: float):
    """ Flash a HEX image with teensy_loader_cli, waiting for the HalfKay bootloader """
    run_tool(["teensy_loader_cli", f"--mcu={mcu}", "-w", "-v", str(image)], timeout)


def flash_bossac(image: Path, bootloader_port: str, timeout: float, touched_port: Optional[str] = None):
    """ Flash a BIN image with bossac once the BOSSA bootloader port is available

    When the bootloader reuses the touched port, wait for the application's port to go away first.
    """
    if touched_port == bootloader_port:
        wait_for(lambda: not Path(bootloader_port).exists(), timeout)
    if not wait_for(lambda: Path(bootloader_port).exists(), timeout):
        raise TouchFlashError(f"Bootloader port {bootloader_port} did not appear within {timeout}s")
    run_tool(["bossac", "-p", bootloader_port, "-e", "-w", "-v", "-R", str(image)], timeout)


def touch_and_flash(args: argparse.Namespace):
    """ Touch the port (unless skipped) then flash with the selected method """
    image = Path(args.image)
    if not image.is_file():
        raise TouchFlashError(f"No such image: {image}")
    if args.method not in METHODS:
        raise TouchFlashError(f"No flashing implementation for method {args.method}")
    existing = find_uf2_volumes() if args.method == "uf2" and not args.volume else []
    if args.port is not None:
        touch(args.port, args.touch_baud if args.touch_baud is not None else METHODS[args.method])
    if args.method == "uf2":
        flash_uf2(image, Path(args.volume) if args.volume else None, args.timeout, existing)
    elif args.method == "teensy":
        flash_teensy(image, args.mcu, args.timeout)
    else:
        flash_bossac(image, args.bootloader_port or args.port, args.timeout, args.port)


def parse_args(arguments: Optional[List[str]] = None) -> argparse.Namespace:
    """ Parse command line arguments for fprime-zephyr-flash """
    parser = argparse.ArgumentParser(
        description="Reboot a board into its bootloader with a baud rate touch, then flash an image",
    )
    parser.add_argument("image", help="Image to flash: .uf2 (uf2), .hex (teensy), or .bin (bossac)")
    parser.add_argument("--method", choices=sorted(METHODS), default="uf2",
                        help="uf2: RP2040/RP2350/nRF52/SAMD UF2 volume; teensy: teensy_loader_cli; bossac: SAMD BOSSA")
    parser.add_argument("--port", default=None,
                        help="Serial port of the running board to touch. Omit when already in the bootloader")
    parser.add_argument("--touch-baud", type=int, default=None,
                        help=f"Touch baud rate. Default: {TEENSY_TOUCH_BAUD} for teensy, {DEFAULT_TOUCH_BAUD} otherwise")
    parser.add_argument("--volume", default=None,
                        help="UF2 volume mount point. Default: search " + ", ".join(UF2_SEARCH_ROOTS))
    parser.add_argument("--mcu", default="TEENSY41", help="teensy_loader_cli MCU. Default: TEENSY41")
    parser.add_argument("--bootloader-port", default=None,
                        help="bossac: serial port of the bootloader when it differs from --port")
    parser.add_argument("--timeout", type=float, default=30.0, help="Seconds to wait for the bootloader. Default: 30")
    parser.add_argument("--verbose", "-v", action="store_true", help="Log progress")
    args = parser.parse_args(arguments)
    if args.method == "bossac" and args.port is None and args.bootloader_port is None:
        parser.error("bossac requires --port or --bootloader-port")
    return args


def main(arguments: Optional[List[str]] = None) -> int:
    """ Entry point for fprime-zephyr-flash """
    args = parse_args(arguments)
    logging.basicConfig(level=logging.INFO if args.verbose else logging.WARNING, format="%(message)s")
    try:
        touch_and_flash(args)
    except TouchFlashError as exception:
        print(f"[ERROR] {exception}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
