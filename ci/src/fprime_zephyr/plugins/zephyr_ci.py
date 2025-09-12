""" fprime_zephyr.plugins.zephyr_ci: Zephyr CI implementation

This module allows Zephyr to build as part of the fprime.ci package
"""
from abc import abstractmethod, ABC
import logging
from pathlib import Path
import serial

import fprime_gds.plugin.definitions
from fprime_ci.ci import Ci
from fprime_ci.plugin.definitions import plugin
from fprime_ci.utilities import IOLogger

LOGGER = logging.getLogger(__name__)


class ZephyrCiBase(Ci, ABC):
    """ Zephyr CI plugin """
    class Keys(Ci.Keys):
        """ Additional keys used during the execution of the Zephyr plugin

        These keys are used as constants when accessing context and used to validate context automatically. These keys
        are supplied in the initial supplied context. To fully understand these keys, see: CiPlugin.Keys for a better
        description of the usage and format.
        """
        FLASH_COMMAND = "flash-command"
        FLASH_COMMAND__ATTRS__ = (False, list)

    def __init__(self, port:str):
        """ Initialize basic components """
        self.port = port
        self.monitor_fsw_thread = None

    @abstractmethod
    def get_file_handle(self):
        """ Get a file-like object to read Zephyr output from """
        pass

    def wait_for_boot(self, file_handle, prompt="*** Booting Zephyr OS build"):
        """ Wait for the Zephyr boot to complete
        
        Args:
            file_handle: file-like object to read Zephyr output from
            prompt: string to wait for in the output
        """
        IOLogger.communicate(
            [file_handle],
            [IOLogger(None, logging.DEBUG, logger_name=f"[ZephyrConsole]")],
            timeout=20.0,
            end=lambda line, index: prompt in line and line.endswith("\n"),
            close=False,
        )
    
    def monitor_fsw_run(self, file_handle):
        """ Monitor the FSW running on the target hardware """
        self.monitor_fsw_thread = IOLogger.async_communicate(
            [file_handle],
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
        flash_command = context.get(self.Keys.FLASH_COMMAND, None)
        process, _, (_, _) = self.subprocess(
            flash_command
        )
        # Make sure that the console port is openable then open it
        self.wait_until(lambda: Path(self.port).exists(), timeout=1.0)
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
            self.wait_for_boot(self.get_file_handle())
            self.monitor_fsw_run(self.get_file_handle())
        except serial.SerialException as exception:
            raise Exception(f"Failed to read: {exception}")
        return context

    def cleanup(self, context: dict):
        """ Cleanup """
        try:
            if self.monitor_fsw_thread is not None:
                IOLogger.join_communicate(self.monitor_fsw_thread)
        finally:
            try:
                self.get_file_handle().close()
            except Exception as e:
                pass
        return context


@plugin(Ci)
class ZephyrCi(ZephyrCiBase):
    """ Zephyr CI plugin using a split console and GDS interface port """

    def __init__(self, port:str, baud:int=None, flow:str="no"):
        """ Set up the Zephyr CI port """
        super().__init__(port)
        self.serial_port = serial.Serial()
        self.serial_port.port = port
        self.serial_port.baudrate = baud
        self.serial_port.rtscts = flow == "yes"

    def get_file_handle(self):
        """ Get the port """
        assert self.serial_port.is_open, "Serial port is not open"
        return self.serial_port

    def preload(self, context):
        """ Preload the Zephyr CI then open the port """
        context = super().preload(context)
        self.serial_port.open()
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
                "help": "Serial port for console logs, or file receiving console logs",
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


@plugin(Ci)
class ZephyrCiUnified(ZephyrCiBase):
    """ Zephyr CI plugin using a unified console and GDS interface port """

    def __init__(self, port:str, unframed_output:str):
        """ Set up the Zephyr CI port """
        super().__init__(port)
        self.unframed_output = unframed_output
        self.file_handle = None

    def get_file_handle(self):
        """ Get the file handle """
        assert self.file_handle is not None, "File handle never opened"
        assert not self.file_handle.closed, "File handle is closed"
        return self.file_handle

    def launch(self, context):
        """ Preload the Zephyr CI then open the port """
        self.file_handle = open(self.unframed_output, "rb")
        assert not self.file_handle.closed, "Failed to open unframed output file"
        return super().launch(context)

    @classmethod
    def get_name(cls):
        """ Returns the name of the plugin """
        return "zephyr-ci-unified"

    @classmethod
    def get_arguments(cls):
        """ Returns the arguments of the plugin """
        return {
            ("--port",): {
                "type": str,
                "default": "/dev/ttyUSB1",
                "help": "Serial port for unified console logs and GDS interface",
            },
            ("--unframed-output",): {
                "type": str,
                "required": True,
                "help": "Path to the unframed data output file",
            }
        }