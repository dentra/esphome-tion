import logging
import time

from emu.base import Transport

from ..base import Emu, Exchange, Protocol, Transport
from .commands import *
from .device import Device3S
from .packet import Packet, Packet3S
from .protocol import Protocol3S


class Emu3S(Emu):
    def __init__(
        self, transport: Transport, disabled_log: list[str] = None, port_name=""
    ) -> None:
        super().__init__(
            port_name,
            Protocol3S(transport, port_name=port_name),
            Device3S(disabled_log),
        )
