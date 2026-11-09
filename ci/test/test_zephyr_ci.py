""" Tests for the fprime_zephyr.plugins.zephyr_ci touch-baud preload """
import importlib
import logging
import sys
import types
from pathlib import Path
from unittest import mock

import pytest

from fprime_zephyr.touch import TEENSY_TOUCH_BAUD


@pytest.fixture
def zephyr_ci():
    """ Import zephyr_ci with stand-ins for fprime-ci and fprime-gds, which are not installable from PyPI here """
    class Ci:
        class Keys:
            pass

    modules = {name: types.ModuleType(name) for name in [
        "fprime_gds", "fprime_gds.plugin", "fprime_gds.plugin.definitions",
        "fprime_ci", "fprime_ci.ci", "fprime_ci.plugin", "fprime_ci.plugin.definitions", "fprime_ci.utilities",
    ]}
    modules["fprime_ci.ci"].Ci = Ci
    modules["fprime_ci.plugin.definitions"].plugin = lambda _: (lambda cls: cls)
    modules["fprime_ci.utilities"].IOLogger = mock.Mock()
    with mock.patch.dict(sys.modules, modules):
        sys.modules.pop("fprime_zephyr.plugins.zephyr_ci", None)
        yield importlib.import_module("fprime_zephyr.plugins.zephyr_ci")
    sys.modules.pop("fprime_zephyr.plugins.zephyr_ci", None)


def make_plugin(zephyr_ci, port):
    plugin = zephyr_ci.ZephyrCiUnified(str(port), "unframed")
    plugin.subprocess = mock.Mock(return_value=(None, None, (None, None)))
    plugin.wait_until = mock.Mock()
    return plugin


def test_preload_touches_and_warns_when_port_stays(zephyr_ci, tmp_path, caplog):
    port = tmp_path / "ttyACM0"
    port.touch()
    plugin = make_plugin(zephyr_ci, port)
    order = mock.Mock()
    plugin.subprocess = order.subprocess
    plugin.subprocess.return_value = (None, None, (None, None))
    with mock.patch.object(zephyr_ci, "touch", order.touch), \
            mock.patch.object(zephyr_ci.ZephyrCiBase, "TOUCH_DISCONNECT_TIMEOUT", 0.2), \
            mock.patch.object(zephyr_ci, "wait_for", wraps=zephyr_ci.wait_for) as wait_mock, \
            caplog.at_level(logging.WARNING):
        plugin.preload({"touch-baud": 1200, "flash-command": ["true"]})
    assert wait_mock.call_args.kwargs["timeout"] == 0.2
    assert order.mock_calls == [mock.call.touch(str(port), 1200), mock.call.subprocess(["true"])]
    assert "still present 0.2 s" in caplog.text


def test_preload_touch_without_warning_when_port_goes_away(zephyr_ci, tmp_path, caplog):
    port = tmp_path / "ttyACM0"
    port.touch()
    plugin = make_plugin(zephyr_ci, port)
    with mock.patch.object(zephyr_ci, "touch", side_effect=lambda *_: port.unlink()) as touch_mock, \
            caplog.at_level(logging.WARNING):
        plugin.preload({"touch-baud": 1200, "flash-command": ["true"]})
    touch_mock.assert_called_once_with(str(port), 1200)
    assert "still present" not in caplog.text
    plugin.subprocess.assert_called_once_with(["true"])


def test_preload_does_not_touch_absent_port(zephyr_ci, tmp_path):
    plugin = make_plugin(zephyr_ci, tmp_path / "ttyACM0")
    with mock.patch.object(zephyr_ci, "touch") as touch_mock:
        plugin.preload({"touch-baud": 1200, "flash-command": ["true"]})
    touch_mock.assert_not_called()
    plugin.subprocess.assert_called_once_with(["true"])


def test_preload_without_touch_baud_does_not_touch(zephyr_ci, tmp_path):
    port = tmp_path / "ttyACM0"
    port.touch()
    plugin = make_plugin(zephyr_ci, port)
    with mock.patch.object(zephyr_ci, "touch") as touch_mock:
        plugin.preload({"flash-command": ["true"]})
    touch_mock.assert_not_called()
    plugin.subprocess.assert_called_once_with(["true"])


SAMPLE_CONFIGS = Path(__file__).resolve().parent.parent / "sample-configs"


def test_teensy41_sample_config_touches_at_teensy_baud():
    yaml = pytest.importorskip("yaml")
    config = yaml.safe_load((SAMPLE_CONFIGS / "teensy41.yml").read_text())
    assert config["touch-baud"] == TEENSY_TOUCH_BAUD
    assert config["flash-command"][0] == "teensy_loader_cli"


def test_pico2_sample_config_flashes_over_debug_probe_without_touch():
    yaml = pytest.importorskip("yaml")
    config = yaml.safe_load((SAMPLE_CONFIGS / "pico2.yml").read_text())
    assert "touch-baud" not in config
    assert config["flash-command"][0] == "openocd"
