import logging

from ..base import CmdId, Command, Packet
from .commands import ALL_COMMANDS, CmdId3S

_LOGGER = logging.getLogger()

FRAME_MAGIC_REQ = 0x3D
FRAME_MAGIC_RSP = 0xB3
FRAME_MAGIC_END = 0x5A
MAX_CMD_SIZE = 18


class Packet3S(Packet):
    def __init__(self, raw: bytes) -> None:
        super().__init__(raw)

    @property
    def data(self) -> bytes:
        return self._raw[1:-1]

    @property
    def valid(self) -> bool:
        return self._raw[-1] == FRAME_MAGIC_END

    @property
    def cmd(self) -> CmdId3S:
        return ALL_COMMANDS.get(self._raw[1], None)

    def raw_update(self, data: bytes) -> None:
        self._raw = self._raw + data

    @classmethod
    def build(cls, cmd: CmdId, data: bytes):
        if not isinstance(cmd, CmdId3S):
            raise ValueError(f"cmd must be a {CmdId3S} class, but not {type(cmd)}")

        size = 1 + len(data)
        if size != cmd.size:
            raise ValueError(f"Invalid response {cmd} size {size}, requred {cmd.size}")

        buf = bytes([cmd.code]) + data
        return cls(buf + bytes([_crc8(buf)]))

    @classmethod
    def build_req(cls, cmd: Command) -> "Packet3S":
        return Packet3S.build(*cmd.mk_req())

    @classmethod
    def build_rsp(cls, cmd: Command) -> "Packet3S":
        return Packet3S.build(*cmd.mk_rsp())

    @classmethod
    def build_set(cls, cmd: Command) -> "Packet3S":
        return Packet3S.build(*cmd.mk_set())
