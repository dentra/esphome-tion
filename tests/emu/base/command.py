import abc
import enum
import logging

_LOGGER = logging.getLogger()


class Flags(enum.IntFlag):
    def __str__(self) -> str:
        value = repr(self)
        value = value[len(type(self).__name__) + 2 : value.index(":")]
        return value


def set_flag(flags: Flags, flag: Flags, value: bool) -> Flags:
    if value:
        flags |= flag
    else:
        flags &= ~(flag)
    return flags


def get_flag(flags: Flags, flag: Flags) -> bool:
    return flags & flag != 0


class CmdId:
    def __init__(self, code: int, name: str = None) -> None:
        self._code = code
        self._name = f"{name}[{code:02X}]" if name else f"COMMAND[{code:02X}]"

    @property
    def code(self) -> int:
        return self._code

    @property
    def name(self) -> str:
        return self._name

    def __str__(self) -> str:
        return self.name

    def __eq__(self, __value: object) -> bool:
        if __value is None:
            return False
        if isinstance(__value, CmdId):
            return self.code == __value.code
        if isinstance(__value, int):
            return self.code == __value
        if isinstance(__value, float):
            return self.code == int(__value)
        if isinstance(__value, str) and __value.endswith("]"):
            return self.code == int(__value[-3:-1], 16)
        return False

    def __hash__(self) -> int:
        return self.code.__hash__()


class Command(abc.ABC):
    cmd_req: CmdId = None
    cmd_rsp: CmdId = None
    cmd_set: CmdId = None

    def __init__(self, data: bytes = None) -> None:
        if data:
            self.upd(data)

    def get(self) -> bytes:
        return None

    def set(self, data: bytes) -> bytes:
        return None

    def upd(self, data: bytes) -> None:
        pass

    def mk_req(self) -> tuple[CmdId, bytes]:
        return None

    def mk_rsp(self) -> tuple[CmdId, bytes]:
        return None

    def mk_set(self) -> tuple[CmdId, bytes]:
        return None

    @staticmethod
    def _ctl_int(name: str, current: int, min: int, max: int, up: bool) -> int:
        newval = current
        if up:
            newval = newval + 1
            if newval > max:
                newval = min
        else:
            newval = newval - 1
            if newval < min:
                newval = max
        _LOGGER.info("%s %d -> %d", name, current, newval)
        return newval

    @staticmethod
    def _ctl_bool(name: str, current: bool) -> bool:
        newval = not current
        _LOGGER.info("%s %d -> %d", name, current, newval)
        return newval

    def __repr__(self) -> str:
        return (
            f"{self.__class__.__name__}[{self.cmd_req},{self.cmd_rsp},{self.cmd_rsp}]"
        )

    def __str__(self) -> str:
        return self.__repr__()
