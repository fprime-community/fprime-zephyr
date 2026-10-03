""" Tests for fprime_zephyr.touch """
from pathlib import Path
from unittest import mock

import pytest

from fprime_zephyr import touch


def make_uf2_volume(path: Path) -> Path:
    path.mkdir(parents=True)
    (path / touch.UF2_INFO_FILE).write_text("UF2 Bootloader\n")
    return path


def test_find_uf2_volumes(tmp_path):
    volume = make_uf2_volume(tmp_path / "user" / "RPI-RP2")
    (tmp_path / "user" / "OTHER").mkdir()
    assert touch.find_uf2_volumes([str(tmp_path), str(tmp_path / "missing")]) == [volume]


def test_flash_uf2_copies_image(tmp_path):
    volume = make_uf2_volume(tmp_path / "RP2350")
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    assert touch.flash_uf2(image, volume, timeout=0.1) == volume
    assert (volume / "zephyr.uf2").read_bytes() == b"UF2\n"


def test_flash_uf2_times_out(tmp_path):
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    with pytest.raises(touch.TouchFlashError):
        touch.flash_uf2(image, tmp_path / "absent", timeout=0.1)


def test_flash_uf2_rejects_other_images(tmp_path):
    image = tmp_path / "zephyr.hex"
    image.write_bytes(b":00000001FF\n")
    with pytest.raises(touch.TouchFlashError):
        touch.flash_uf2(image, None, timeout=0.1)


@pytest.mark.parametrize("method,baud", [("uf2", 1200), ("bossac", 1200), ("teensy", 134)])
def test_default_touch_baud(tmp_path, method, baud):
    image = tmp_path / "zephyr.img"
    image.write_bytes(b"")
    args = touch.parse_args([str(image), "--method", method, "--port", "/dev/ttyACM0"])
    with mock.patch.object(touch, "touch") as touch_mock, \
            mock.patch.object(touch, "flash_uf2"), \
            mock.patch.object(touch, "flash_teensy"), \
            mock.patch.object(touch, "flash_bossac"):
        touch.touch_and_flash(args)
    touch_mock.assert_called_once_with("/dev/ttyACM0", baud)


def test_touch_baud_override_and_no_port(tmp_path):
    image = tmp_path / "zephyr.hex"
    image.write_bytes(b"")
    with mock.patch.object(touch, "touch") as touch_mock, mock.patch.object(touch, "run_tool") as tool_mock:
        touch.touch_and_flash(touch.parse_args([str(image), "--method", "teensy", "--mcu", "TEENSY40"]))
        touch.touch_and_flash(touch.parse_args([str(image), "--method", "teensy", "--port", "p", "--touch-baud", "300"]))
    touch_mock.assert_called_once_with("p", 300)
    assert tool_mock.call_args_list[0].args[0] == ["teensy_loader_cli", "--mcu=TEENSY40", "-w", "-v", str(image)]


def test_touch_opens_port_at_baud():
    with mock.patch.object(touch.serial, "Serial") as serial_mock:
        touch.touch("/dev/ttyACM0", 1200, settle=0)
    serial_mock.assert_called_once_with("/dev/ttyACM0", baudrate=1200)
    serial_mock.return_value.close.assert_called_once()


def test_main_reports_missing_image(tmp_path, capsys):
    assert touch.main([str(tmp_path / "missing.uf2")]) == 1
    assert "No such image" in capsys.readouterr().err
