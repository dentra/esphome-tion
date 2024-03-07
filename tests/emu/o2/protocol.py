import logging
import queue
import struct

from ..base import CmdId, Protocol, Transport
from .commands import *
from .packet import PacketO2

_LOGGER = logging.getLogger()


class ProtocolO2(Protocol):
    def __init__(self, transport: Transport, port_name="") -> None:
        self.transport = transport
        self.port_name = f"[{port_name}] " if port_name else ""

    log_filter: set[bytes] = set()

    def _has_filter(self, pkt: PacketO2) -> bool:
        if pkt.raw in self.log_filter:
            return True

        # code = pkt.cmd.code
        code = pkt.raw[0]

        if code == CMD_TIME_GET_RSP:
            return True

        if code == CMD_STATE_GET_RSP:
            try:
                state = StateCommand(pkt.data)
                if state.unknown7 != 4 or state.error != 0 or state.unknown9 != 0:
                    return False
            except struct.error as e:
                _LOGGER.warning("%s: %s", e, pkt.log_raw)
            return True

        self.log_filter.add(pkt.raw)
        return False

    last_write = None

    def tx(self, pkt: PacketO2) -> None:
        if not self._has_filter(pkt):
            _LOGGER.debug("%sTX: %s", self.port_name, pkt.log_raw)

        self.transport.write(pkt.raw)

        self.last_write = pkt

    def rx(self) -> PacketO2:
        def skip(description: str, pkt: PacketO2, do_log: bool):
            buf = pkt.raw + self.transport.read(self.transport.available)
            if do_log:
                _LOGGER.warning("%sRX: %s", self.port_name, description)
                _LOGGER.warning("  Skip: %s (%d)", buf.hex(" ").upper(), len(buf))
                _LOGGER.warning("      : %s", buf.decode(errors="replace"))

        if not self.transport.available:
            return None

        pkt = PacketO2(self.transport.read(1))
        if not pkt.raw:
            return None

        if pkt.raw[0] == 0xFF:  ## Первая команда. Возможно остатки от CONNECT: 00 FF
            return None

        if pkt.cmd is None:
            skip(f"Unknown command for last {self.last_write}", pkt, True)
            return None

        pkt.raw_update(self.transport.read(pkt.cmd.size))
        if not pkt.valid:
            skip(f"Invalid CRC", pkt, False)
            return None

        if not self._has_filter(pkt):
            _LOGGER.debug("%sRX: %s", self.port_name, pkt.log_raw)

        return pkt
