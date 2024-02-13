import abc
import logging
import typing

from .command import CmdId, Command
from .packet import Packet

_LOGGER = logging.getLogger()


class Device(abc.ABC):

    def __init__(self, commands: list[Command], disabled_log: list[str]):
        self._commands = commands
        self._disabled_log = disabled_log
        _LOGGER.info("commands: %s", self._commands)

    def _find(
        self, cmd: CmdId, chk_fn: typing.Callable[[Command], CmdId]
    ) -> typing.Optional[Command]:
        for sup in self._commands:
            if cmd == chk_fn(sup):
                return sup
        return None

    def _log(self, cmd: CmdId, data: bytes):
        if cmd in self._disabled_log:
            return
        if data:
            _LOGGER.info("%s: %s (%d)", cmd, data.hex(" "), len(data))
        else:
            _LOGGER.info(cmd)

    @abc.abstractmethod
    def pkt(self, cmd: CmdId, data: bytes) -> Packet:
        pass

    def req(self, pkt: Packet) -> Packet:
        def rsp(cmd: Command, data: bytes) -> Packet:
            # self._log(cmd.cmd_rsp, data)
            pkt = self.pkt(cmd.cmd_rsp, data)
            assert pkt
            return pkt

        req = self._find(pkt.cmd, lambda c: c.cmd_req)
        if req:
            return rsp(req, req.get())

        set = self._find(pkt.cmd, lambda c: c.cmd_set)
        if set:
            return rsp(set, set.set(pkt.data))

        _LOGGER.warning("Unsupported command %s: %s", pkt.cmd, pkt.log_data)
        return None

    def upd(self, pkt: Packet) -> bool:
        lst: list[typing.Callable[[Command], CmdId]] = [
            # lambda c: c.cmd_req,
            lambda c: c.cmd_set,
            lambda c: c.cmd_rsp,
        ]
        for itm in lst:
            res = self._find(pkt.cmd, itm)
            if res:
                res.upd(pkt.data)
                # _LOGGER.info("Updating %s with %s", res, pkt)
                return True
        _LOGGER.warning("Can't find command for %s", pkt)
        return False

    def instance(self, cls):
        for cmd in self._commands:
            if isinstance(cmd, cls):
                return cmd
        return None
