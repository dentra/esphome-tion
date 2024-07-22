import logging

from emu.base.packet import Packet

from ..base import CmdId, Device
from .commands import *
from .packet import Packet3S

_LOGGER = logging


class Device3S(Device):
    def __init__(self, disabled_log: list[str]):
        self.state = StateCommand()
        self.pair = PairCommand()
        super().__init__(
            [
                self.state,
                self.pair,
            ],
            disabled_log,
        )

    def pkt(self, cmd: CmdId, data: bytes):
        return Packet3S.build(cmd, data)
