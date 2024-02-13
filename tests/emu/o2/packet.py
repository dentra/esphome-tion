import logging

from ..base import CmdId, Command, Packet
from .commands import ALL_COMMANDS, CmdIdO2

_LOGGER = logging.getLogger()


def _crc8(data: bytes, init: int = 0xFF):
    crc = init
    for val in data:
        crc = crc ^ val
    return crc & 0xFF


class PacketO2(Packet):
    def __init__(self, raw: bytes) -> None:
        super().__init__(raw)

    @property
    def data(self) -> bytes:
        return self._raw[1:-1]

    @property
    def valid(self) -> bool:
        return _crc8(self._raw) == 0

    @property
    def cmd(self) -> CmdIdO2:
        return ALL_COMMANDS.get(self._raw[0], None)

    def raw_update(self, data: bytes) -> None:
        self._raw = self._raw + data

    @classmethod
    def build(cls, cmd: CmdId, data: bytes):
        if not isinstance(cmd, CmdIdO2):
            raise ValueError(f"cmd must be a {CmdIdO2} class, but not {type(cmd)}")

        size = 1 + len(data)
        if size != cmd.size:
            raise ValueError(f"Invalid response {cmd} size {size}, requred {cmd.size}")

        buf = bytes([cmd.code]) + data
        return cls(buf + bytes([_crc8(buf)]))

    @classmethod
    def build_req(cls, cmd: Command) -> "PacketO2":
        return PacketO2.build(*cmd.mk_req())

    @classmethod
    def build_rsp(cls, cmd: Command) -> "PacketO2":
        return PacketO2.build(*cmd.mk_rsp())

    @classmethod
    def build_set(cls, cmd: Command) -> "PacketO2":
        return PacketO2.build(*cmd.mk_set())
