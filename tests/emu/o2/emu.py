import logging
import time

from emu.base import Transport

from ..base import Emu, Exchange, Protocol, Transport
from .commands import *
from .device import DeviceO2
from .packet import Packet, PacketO2
from .protocol import ProtocolO2

_LOGGER = logging.getLogger()

PROXY_COMMANDS = {
    # CMD_CONNECT_REQ,
    # CMD_CONNECT_RSP,
    # CMD_DEV_INFO_REQ,
    # CMD_DEV_INFO_RSP,
    # CMD_DEV_MODE_REQ,
    # CMD_DEV_MODE_RSP,
    # CMD_SET_WORK_MODE_REQ,
    # CMD_SET_WORK_MODE_RSP,
    # CMD_STATE_GET_REQ,
    # CMD_STATE_GET_RSP,
    # CMD_STATE_SET_REQ,
}


class EmuO2(Emu):
    def __init__(
        self, transport: Transport, disabled_log: list[str] = None, port_name=""
    ) -> None:
        super().__init__(
            port_name,
            ProtocolO2(transport, port_name=port_name),
            DeviceO2(disabled_log),
        )


class _EmuO2Proxy(EmuO2):
    def __init__(
        self,
        rf_port: Transport,
        o2_port: Transport,
        disabled_log: list[str] = None,
        rf_port_name="RF",
        o2_port_name="O2",
    ) -> None:
        super().__init__(rf_port, disabled_log=disabled_log, port_name=rf_port_name)
        self.o2 = Exchange(
            o2_port_name, ProtocolO2(o2_port, port_name=o2_port_name), self.alive
        )
        self.rf_port_name = rf_port_name
        self.o2_port_name = o2_port_name

    @property
    def rf(self) -> Exchange:
        return self.reader

    def start(self) -> bool:
        if not super().start():
            return False
        self.o2.start()
        return True


class EmuO2Proxy(_EmuO2Proxy):
    def __init__(
        self, rf_port: Transport, o2_port: Transport, disabled_log: list[str] = None
    ) -> None:
        super().__init__(rf_port, o2_port, disabled_log)

    def _loop_o2(self):
        pkt = self.o2.read()
        if not pkt:
            return
        self.device.upd(pkt)
        self.log_tion_rx(pkt)
        if pkt.cmd in PROXY_COMMANDS:
            self.rf.write(pkt)

    def _loop_rf(self):
        pkt = self.rf.read()
        if not pkt:
            return

        rsp = self.device.req(pkt)
        if pkt.cmd in PROXY_COMMANDS:
            self.o2.write(pkt)
            return

        if rsp:
            self.rf.write(rsp)
        else:
            _LOGGER.warning("[%s] TX Unknown %s", self.o2_port_name, pkt)
            self.o2.write(pkt)

    def loop(self):
        self._loop_o2()
        self._loop_rf()

    _o2_filter: set[bytes] = set()
    _o2_dev_mode = 0xFF

    def log_tion_rx(self, pkt: Packet):
        if pkt.raw in self._o2_filter:
            return

        if pkt.cmd == CMD_DEV_MODE_RSP:
            dev_mode = DevModeCommand(pkt.data)
            if self._o2_dev_mode != dev_mode.flags:
                _LOGGER.info("O2 new value %s: %s", pkt.cmd, dev_mode.flags)
                self._o2_dev_mode = dev_mode.flags
            return

        self._o2_filter.add(pkt.raw)

        _LOGGER.info("O2 new value: %s", pkt)


class EmuO2Dev(EmuO2):
    """Эмулирет клиента, подключенного непостредственно к бризеру."""

    def __init__(self, o2_port: Transport, disabled_log: list[str] = None) -> None:
        super().__init__(o2_port, disabled_log, "O2")

    @property
    def o2(self) -> Exchange:
        return self.reader

    @property
    def device(self) -> DeviceO2:
        return self._device

    def loop(self):
        if pkt := self.o2.read():
            cmd = self.device.upd_cmd(pkt)
            _LOGGER.info(str(cmd))

    def request_connect(self):
        self.o2.write(PacketO2.build_req(self.device.connect))

    def request_dev_info(self):
        self.o2.write(PacketO2.build_req(self.device.dev_info))

    def request_dev_mode(self):
        self.o2.write(PacketO2.build_req(self.device.dev_mode))

    def request_state(self):
        self.o2.write(PacketO2.build_req(self.device.state))

    def write_state(self):
        self.o2.write(PacketO2.build_set(self.device.state))

    def write_work_mode(self):
        self.o2.write(PacketO2.build_set(self.device.work_mode))
