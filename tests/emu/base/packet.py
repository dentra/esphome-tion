import abc

from .command import CmdId


class Packet(abc.ABC):
    def __init__(self, raw: bytes) -> None:
        self._raw = raw

    @property
    def raw(self) -> bytes:
        return self._raw

    @staticmethod
    def _log(buf: bytes) -> str:
        if buf:
            return f"{buf.hex(' ').upper()} ({len(buf)})"
        else:
            return "[null]"

    @property
    def log_raw(self) -> str:
        return self._log(self.raw)

    @property
    def log_data(self) -> str:
        return self._log(self.data)

    @property
    @abc.abstractproperty
    def data(self) -> bytes:
        pass

    @property
    @abc.abstractproperty
    def cmd(self) -> CmdId:
        pass

    @property
    @abc.abstractproperty
    def valid(self) -> bool:
        pass

    def __str__(self) -> str:
        return f"{self.cmd}: {self.log_data}"
