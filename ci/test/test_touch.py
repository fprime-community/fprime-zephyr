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
            mock.patch.object(touch, "find_uf2_volumes", return_value=[]), \
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


def test_find_uf2_volumes_skips_symlinks(tmp_path):
    make_uf2_volume(tmp_path / "real" / "RPI-RP2")
    (tmp_path / "link").symlink_to(tmp_path / "real")
    assert touch.find_uf2_volumes([str(tmp_path)]) == [tmp_path / "real" / "RPI-RP2"]


def test_find_uf2_volumes_skips_unreadable(tmp_path):
    volume = make_uf2_volume(tmp_path / "RP2350")
    with mock.patch.object(touch, "_subdirectories", side_effect=[[volume, tmp_path / "locked"], [], []]):
        assert touch.find_uf2_volumes([str(tmp_path)]) == [volume]
    assert touch._subdirectories(tmp_path / "missing") == []


def test_find_uf2_volumes_skips_child_that_cannot_be_stat(tmp_path):
    volume = make_uf2_volume(tmp_path / "RPI-RP2")
    (tmp_path / "locked").mkdir()
    real_is_dir = touch.Path.is_dir

    def is_dir(self):
        if self.name == "locked":
            raise PermissionError(13, "Permission denied", str(self))
        return real_is_dir(self)

    with mock.patch.object(touch.Path, "is_dir", is_dir):
        assert touch.find_uf2_volumes([str(tmp_path)]) == [volume]


def test_unknown_method_fails_before_touch(tmp_path):
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    args = touch.parse_args([str(image), "--port", "/dev/ttyACM0"])
    args.method = "dfu"
    with mock.patch.object(touch, "touch") as touch_mock, \
            pytest.raises(touch.TouchFlashError, match="No flashing implementation"):
        touch.touch_and_flash(args)
    touch_mock.assert_not_called()


def test_flash_uf2_ignores_existing_volume(tmp_path):
    stale = make_uf2_volume(tmp_path / "STALE")
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    with mock.patch.object(touch, "find_uf2_volumes", return_value=[stale]):
        with pytest.raises(touch.TouchFlashError):
            touch.flash_uf2(image, None, timeout=0.1, existing=[stale])
    assert not (stale / "zephyr.uf2").exists()


def test_flash_uf2_rejects_several_new_volumes(tmp_path):
    volumes = [make_uf2_volume(tmp_path / "A"), make_uf2_volume(tmp_path / "B")]
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    with mock.patch.object(touch, "find_uf2_volumes", return_value=volumes):
        with pytest.raises(touch.TouchFlashError, match="--volume"):
            touch.flash_uf2(image, None, timeout=0.1)


def test_flash_uf2_does_not_follow_destination_symlink(tmp_path):
    volume = make_uf2_volume(tmp_path / "RPI-RP2")
    victim = tmp_path / "victim"
    victim.write_bytes(b"keep")
    (volume / "zephyr.uf2").symlink_to(victim)
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    with pytest.raises(OSError):
        touch.flash_uf2(image, volume, timeout=0.1)
    assert victim.read_bytes() == b"keep"


def test_flash_uf2_snapshots_volumes_before_touch(tmp_path):
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    stale = tmp_path / "STALE"
    calls = mock.Mock()
    calls.find_uf2_volumes.return_value = [stale]
    with mock.patch.object(touch, "find_uf2_volumes", calls.find_uf2_volumes), \
            mock.patch.object(touch, "touch", calls.touch), mock.patch.object(touch, "flash_uf2", calls.flash_uf2):
        touch.touch_and_flash(touch.parse_args([str(image), "--port", "/dev/ttyACM0"]))
    assert [call[0] for call in calls.mock_calls] == ["find_uf2_volumes", "touch", "flash_uf2"]
    assert calls.flash_uf2.call_args.args[3] == [stale]


def test_flash_uf2_without_port_accepts_mounted_volume(tmp_path):
    volume = make_uf2_volume(tmp_path / "RPI-RP2")
    image = tmp_path / "zephyr.uf2"
    image.write_bytes(b"UF2\n")
    find = touch.find_uf2_volumes
    with mock.patch.object(touch, "find_uf2_volumes", lambda: find([str(tmp_path)])), \
            mock.patch.object(touch, "touch") as touch_mock:
        touch.touch_and_flash(touch.parse_args([str(image), "--timeout", "0.1"]))
    touch_mock.assert_not_called()
    assert (volume / "zephyr.uf2").read_bytes() == b"UF2\n"


def test_flash_bossac_fails_when_port_stays(tmp_path):
    port = tmp_path / "ttyACM0"
    port.touch()
    with mock.patch.object(touch, "run_tool") as tool_mock:
        with pytest.raises(touch.TouchFlashError, match="did not go away"):
            touch.flash_bossac(tmp_path / "zephyr.bin", str(port), 0.1, str(port))
    tool_mock.assert_not_called()


def test_flash_bossac_waits_for_port_cycle(tmp_path):
    port = tmp_path / "ttyACM0"
    image = tmp_path / "zephyr.bin"
    states = [True, False, True]
    with mock.patch.object(touch.Path, "exists", lambda self: states.pop(0)), \
            mock.patch.object(touch, "run_tool") as tool_mock:
        touch.flash_bossac(image, str(port), 1.0, str(port))
    assert states == []
    assert tool_mock.call_args.args[0][:3] == ["bossac", "-p", str(port)]
