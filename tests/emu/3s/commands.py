import datetime
import enum
import logging
import struct

from ..base import CmdId, Command
from ..base.command import Flags, get_flag, set_flag

_LOGGER = logging.getLogger()

ALL_COMMANDS: dict[int, "CmdId3S"] = {}


class CmdId3S(CmdId):
    def __init__(self, code: int, name: str, size: int) -> None:
        super().__init__(code, name)
        self._size = size

    @property
    def size(self):
        return self._size


class _CmdId3S(CmdId3S):
    def __init__(self, code: int, name: str, size: int) -> None:
        super().__init__(code, name, size)
        ALL_COMMANDS[code] = self


CMD_STATE_GET_REQ = _CmdId3S(0x01, "STATE_GET_REQ")
CMD_STATE_GET_RSP = _CmdId3S(0x10, "STATE_GET_RSP")
CMD_STATE_SET_REQ = _CmdId3S(0x02, "STATE_SET_REQ")

CMD_TIME_REQ = _CmdId3S(0x03, "TIME_REQ")

CMD_TIMERS_REQ = _CmdId3S(0x04, "TIMERS_REQ")

CMD_PAIR_REQ = _CmdId3S(0x05, "PAIR_REQ")
CMD_PAIR_RSP = _CmdId3S(0x50, "PAIR_RSP")

CMD_RESET_REQ = _CmdId3S(0x06, "RESET_REQ")

CMD_CONNECT_REQ = _CmdId3S(0x07, "CONNECT_REQ")

CMD_ALARM_REQ = _CmdId3S(0x09, "ALARM_REQ")

CMD_ALARM_ON_REQ = _CmdId3S(0x0A, "ALARM_ON_REQ")

CMD_ALARM_OFF_REQ = _CmdId3S(0x0B, "ALARM_OFF_REQ")


class Command3S(Command):
    def _ctl_flag(self, name: str, flags: Flags, flag: Flags) -> Flags:
        return set_flag(flags, flag, self._ctl_bool(name, get_flag(flags, flag)))


class StateFlags(Flags):
    HEATER = enum.auto()
    POWER = enum.auto()
    TIMER = enum.auto()
    SOUND = enum.auto()
    MA_AUTO = enum.auto()
    MA_CONNECTED = enum.auto()
    SAVE = enum.auto()
    MA_PAIRING = enum.auto()
    PRESET_STATE = enum.auto()
    PRESETS_STATE = enum.auto()
    UNKNOWN1 = enum.auto()
    UNKNOWN2 = enum.auto()
    UNKNOWN3 = enum.auto()
    UNKNOWN4 = enum.auto()
    UNKNOWN5 = enum.auto()
    UNKNOWN6 = enum.auto()


class StateCommand(Command3S):
    fan_speed = 1
    gate_position = 0
    target_temperature = 10
    flags = 0
    curr_temp = 22
    out_temp = 22
    filter_time = 180
    hours = 0
    minutes = 0

    PRODUCTIVITY_MAP = {1: 34, 2: 59, 3: 74, 4: 119, 5: 120, 6: 121}

    STRUCT_GET = struct.Struct("<BbBB")
    STRUCT_SET = struct.Struct("<Bb??B")

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_STATE_GET_REQ
        self.cmd_rsp = CMD_STATE_GET_RSP
        self.cmd_set = CMD_STATE_SET_REQ

    def get(self) -> bytes:
        _LOGGER.info(
            "Get state: fan=%d, T(t)=%d, flags=%s",
            self.fan_speed,
            self.target_temp,
            self.flags,
        )
        return self.STRUCT_GET.pack(
            self.fan_speed | (self.gate_position << 4),
            self.target_temperature,
            self.flags,
            self.curr_temp,
            self.curr_temp,
            self.out_temp,
            self.filter_time,
            0,  # hours
            0,  # minutes,
            0,  # last_error
            self.PRODUCTIVITY_MAP[self.fan_speed],
            0,  # filter_days
            0,  # firmware_version
        )

    def set(self, data: bytes) -> bytes:
        (
            self.fan_speed,
            self.target_temp,
            self.gate_position,
            flags,
            filter_flags,  # save:1, reset:1, reserved:6
            filter_time,
            factory_reset,
            service_mode,
        ) = self.STRUCT_SET.unpack(data)

        self.flags = StateFlags(flags)

        _LOGGER.info(
            "Set state: fan=%d, T(t)=%d, flags=[%s], src=%s",
            self.fan_speed,
            self.target_temp,
            self.flags,
        )
        return self.get()

    def upd(self, data: bytes) -> None:
        raise NotImplementedError()

    def fan_up(self, up: bool = True):
        self.fan_speed = self._ctl_int("fan_speed", self.fan_speed, 1, 6, up)

    def power_up(self):
        self.flags = self._ctl_flag("power", self.flags, StateFlags.POWER)

    def heat_up(self):
        self.flags = self._ctl_flag("heat", self.flags, StateFlags.HEATER)

    def t_current_up(self, up: bool = True) -> int:
        self.current_temp = self._ctl_int(
            "current_temp", self.current_temp, -20, 25, up
        )

    def t_target_up(self, up: bool = True) -> int:
        self.target_temp = self._ctl_int("target_temp", self.target_temp, -20, 25, up)

    def t_outdoor_up(self, up: bool = True) -> int:
        self.outdoor_temp = self._ctl_int(
            "outdoor_temp", self.outdoor_temp, -20, 25, up
        )

    def mk_req(self) -> tuple[CmdId, bytes]:
        return (self.cmd_req, bytes())

    def mk_set(self) -> tuple[CmdId, bytes]:
        raise NotImplementedError()

    def __str__(self):
        return (
            ""
            + f"(F)an={self.fan_speed}, "
            + f"T(c)={self.current_temp}, "
            + f"T(o)={self.outdoor_temp}, "
            + f"T(t)={self.target_temp}, "
            + f"flags=[{self.flags}], "
        )
