""" fprime_zephyr.plugins.zephyr_ci: Zephyr CI implementation

This module allows Zephyr to build as part of the fprime.ci package
"""
import os
import stat
import time
import logging
import shutil
import subprocess
from enum import Enum
from pathlib import Path
from typing import Type
import serial

import fprime_gds.plugin.definitions
from fprime_ci.ci import Ci
from fprime_ci.plugin.definitions import plugin
from fprime_ci.utilities import IOLogger

LOGGER = logging.getLogger(__name__)

@plugin(Ci)
class ZephyrCi(Ci):
    """ Zephyr CI plugin """
    class Keys(Ci.Keys):
        """ Additional keys used during the execution of the VxWorks plugin

        These keys are used as constants when accessing context and used to validate context automatically. These keys
        are supplied in the initial supplied context. To fully understand these keys, see: CiPlugin.Keys for a better
        description of the usage and format.
        """
        FLASH_RUNNER = "west-flash-runner"

    def __init__(self, port, baud, flow:str="no"):
        """  """
        self.port = serial.Serial()
        self.port.port = port
        self.port.baudrate = baud
        self.port.rtscts = flow == "yes"
        self.monitor_fsw_thread = None

    def wait_for_mainloop(self, prompt="[F Prime] Entering main loop"):
        """ Wait for the Zephyr log "mainloop" to be ready """
        assert self.port.is_open, "Serial port is not open"
        IOLogger.communicate(
            [self.port],
            [IOLogger(None, logging.DEBUG, logger_name=f"[ZephyrConsole]")],
            timeout=20.0,
            end=lambda line, index: prompt in line,
            close=False
        )
    
    def monitor_fsw_run(self):
        """ Wait for the vxprompt to be ready """
        assert self.port.is_open, "Serial port is not open"
        self.monitor_fsw_thread = IOLogger.async_communicate(
            [self.port],
            [IOLogger(None, logging.DEBUG, logger_name=f"[ZephyrConsole]")],
        )

    def preload(self, context: dict):
        """ Load the software to target hardware after power-on

        This function may be overridden by platform developers to perform software loading actions in preparation post
        power-on. This is the most convenient place to copy files via an active program like scp  This step runs
        directly after power-on.

        The default implementation does nothing.

        Note: platforms with long boot times should confirm a successful boot code before attempting load operations.

        TODO: list variables containing software set-up
        Args:
            context: build context aggregated across all build steps
        Returns:
            context optionally augmented with plugin-specific preload data
        """
        process, _, (_, stderr) = self.subprocess(
            ["west", "flash", "--skip-rebuild", "-r", context.get(self.Keys.FLASH_RUNNER), "--build-dir", "../../build-fprime-automatic-zephyr"],
            cwd="./lib/zephyr-workspace", capture=(False, True)
        )
        stderr = stderr.strip()
        if stderr:
            raise Exception(f"'west flash ...' produced stderr: {stderr}")
        # Make sure that the console port is openable then open it
        self.wait_until(lambda: Path(self.port.port).exists(), timeout=1.0)
        self.port.open()
        return context


    def launch(self, context: dict):
        """ Launch the software on the target hardware

        This function must be overridden by platform developers to perform software launching actions. This might
        include running the executable via SSH, passing launch codes to a serial console, restarting the hardware, or
        nothing.

        There is no default implementation for this function, platforms with no explicit launching steps must supply
        a no-op function.

        Args:
            context: build context aggregated across all build steps
        """
        #TODO: wait for acknowledge
        try:
            self.wait_for_mainloop()
            self.monitor_fsw_run()
        except serial.SerialException as exception:
            raise Exception(f"Failed to use serial port: {exception}")
        return context

    def cleanup(self, context: dict):
        """ Cleanup """
        try:
            if self.monitor_fsw_thread is not None:
                IOLogger.join_communicate(self.monitor_fsw_thread)
        finally:
            self.port.close()
        return context

    @classmethod
    def get_name(cls):
        """ Returns the name of the plugin """
        return "zephyr-ci"

    @classmethod
    def get_arguments(cls):
        """ Returns the arguments of the plugin """
        return {
            ("--port",): {
                "type": str,
                "default": "/dev/ttyUSB1",
                "help": "Serial port used to communicate with VxWorks",
            },
            ("--baud",): {
                "type": int,
                "default": 115200,
                "help": "Baud rate for the serial interface",
            },
            ("--flow",): {
                "default": "no",
                "type": str,
                "help": "Whether to enable flow control on the serial interface. Default: no",
            }

        }
