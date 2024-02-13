from emu.base.packet import Packet

from ..base import CmdId, Device
from .commands import *
from .packet import PacketO2

_LOGGER = logging.getLogger()


class DeviceO2(Device):
    def __init__(self, disabled_log: list[str]):
        self.dev_info = DevInfoCommand()
        self.state = StateCommand()
        self.dev_mode = DevModeCommand()
        self.time = TimeCommand()
        self.connect = ConnectCommand()
        self.work_mode = SetWorkModeCommand()
        super().__init__(
            [
                self.dev_info,
                self.state,
                self.dev_mode,
                self.time,
                self.connect,
                self.work_mode,
            ],
            disabled_log,
        )

    def pkt(self, cmd: CmdId, data: bytes):
        return PacketO2.build(cmd, data)

    _dev_mode_state = DevModeFlags(0)

    def req(self, pkt: Packet) -> Packet:
        if pkt.cmd.code == CMD_DEV_INFO_RSP:
            state = DevModeFlags(pkt.data[0])
            if self._dev_mode_state != state:
                _LOGGER.debug("%s: %s", pkt.cmd, state)
                self._dev_mode_state = state
        return super().req(pkt)
