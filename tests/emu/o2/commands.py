import datetime
import enum
import logging
import struct

from ..base import CmdId, Command

_LOGGER = logging.getLogger()

ALL_COMMANDS: dict[int, "CmdIdO2"] = {}


class _FlagsO2(enum.IntFlag):
    def __str__(self) -> str:
        value = repr(self)
        value = value[len(type(self).__name__) + 2 : value.index(":")]
        return value


def set_flag(flags: _FlagsO2, flag: _FlagsO2, value: bool) -> _FlagsO2:
    if value:
        flags |= flag
    else:
        flags &= ~(flag)
    return flags


def get_flag(flags: _FlagsO2, flag: _FlagsO2) -> bool:
    return flags & flag != 0


class CmdIdO2(CmdId):
    def __init__(self, code: int, name: str, size: int) -> None:
        super().__init__(code, name)
        self._size = size

    @property
    def size(self):
        return self._size


class _CmdIdO2(CmdIdO2):
    def __init__(self, code: int, name: str, size: int) -> None:
        super().__init__(code, name, size)
        ALL_COMMANDS[code] = self


CMD_CONNECT_REQ = _CmdIdO2(0x00, "CONNECT_REQ", 1)
"""Command `00`"""
CMD_CONNECT_RSP = _CmdIdO2(0x10, "CONNECT_RSP", 5)
"""Command `10`"""

CMD_STATE_GET_REQ = _CmdIdO2(0x01, "STATE_GET_REQ", 1)
"""Command `01`"""
CMD_STATE_GET_RSP = _CmdIdO2(0x11, "STATE_GET_RSP", 18)
"""Command `11`"""
CMD_STATE_SET_REQ = _CmdIdO2(0x02, "STATE_SET_REQ", 6)
"""Command `02`"""

CMD_DEV_MODE_REQ = _CmdIdO2(0x03, "DEV_MODE_REQ", 1)
"""Command `03`"""
CMD_DEV_MODE_RSP = _CmdIdO2(0x13, "DEV_MODE_RSP", 2)
"""Command `13`"""

CMD_SET_WORK_MODE_REQ = _CmdIdO2(0x04, "SET_WORK_MODE_REQ", 2)
"""Command `04`"""
CMD_SET_WORK_MODE_RSP = _CmdIdO2(0x55, "SET_WORK_MODE_RSP", 1)
"""Command `55`"""

CMD_TIME_GET_REQ = _CmdIdO2(0x05, "TIME_GET_REQ", 1)
"""Command `05`"""
CMD_TIME_GET_RSP = _CmdIdO2(0x15, "TIME_GET_RSP", 4)
"""Command `15`"""
CMD_TIME_SET_REQ = _CmdIdO2(0x06, "TIME_SET_REQ", 4)
"""Command `06`"""

CMD_DEV_INFO_REQ = _CmdIdO2(0x07, "DEV_INFO_REQ", 1)
"""Command `07`"""
CMD_DEV_INFO_RSP = _CmdIdO2(0x17, "DEV_INFO_RSP", 25)
"""Command `17`"""


class _CommandO2(Command):
    def __init__(self, data: bytes = None) -> None:
        super().__init__(data)

    def _ctl_flag(self, name: str, flags: _FlagsO2, flag: _FlagsO2) -> _FlagsO2:
        return set_flag(flags, flag, self._ctl_bool(name, get_flag(flags, flag)))


class TimeCommand(Command):
    """
    Implements commands: `05`, `15`, `06`
    """

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_TIME_GET_REQ
        self.cmd_rsp = CMD_TIME_GET_RSP
        self.cmd_set = CMD_TIME_SET_REQ

    def _get(self, hour: int, minute: int, second: int) -> bytes:
        return bytes([hour, minute, second])

    def get(self) -> bytes:
        now = datetime.datetime.now()
        return self._get(now.hour, now.minute, now.second)

    def set(self, data: bytes) -> bytes:
        return self._get(data[0], data[1], data[2])

    def mk_req(self) -> tuple[CmdId, bytes]:
        return (self.cmd_req, bytes())


class ConnectCommand(Command):
    """
    Implements commands: `00`, `10`
    """

    state = bytes(b"\x04\x10\x01\x00")

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_CONNECT_REQ
        self.cmd_rsp = CMD_CONNECT_RSP

    def get(self) -> bytes:
        # result is unknown and constant
        return self.state

    def upd(self, data: bytes) -> None:
        self.state = data

    def mk_req(self) -> tuple[CmdId, bytes]:
        return (self.cmd_req, bytes())


class DevModeFlags(_FlagsO2):
    PAIR = enum.auto()
    """set when pair enabled"""
    USER = enum.auto()
    """set when user click buttons on breezer"""
    UNKNOWN2 = enum.auto()
    """maby unused"""
    UNKNOWN3 = enum.auto()
    """maby unused"""
    UNKNOWN4 = enum.auto()
    """maby unused"""
    UNKNOWN5 = enum.auto()
    """maby unused"""
    UNKNOWN6 = enum.auto()
    """maby unused"""
    UNKNOWN7 = enum.auto()
    """maby unused"""


class DevModeCommand(_CommandO2):
    """
    Implements commands: `03`, `13`
    """

    flags: DevModeFlags = DevModeFlags(0)

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_DEV_MODE_REQ
        self.cmd_rsp = CMD_DEV_MODE_RSP

    def get(self) -> bytes:
        return bytes([self.flags])

    def upd(self, data: bytes) -> None:
        self.flags = DevModeFlags(data[0])

    def mk_req(self) -> tuple[CmdId, bytes]:
        return (self.cmd_req, bytes())

    def switch_user(self):
        self.flags = self._ctl_flag("dev mode user", self.flags, DevModeFlags.USER)

    def switch_pair(self):
        self.flags = self._ctl_flag("dev mode pair", self.flags, DevModeFlags.PAIR)

    def __str__(self):
        return f"dev_mode={self.flags}"


class WorkModeFlags(_FlagsO2):
    UNKNOWN0 = enum.auto()
    RF_CONNECTED = enum.auto()
    """set when RF module connected"""
    UNKNOWN2 = enum.auto()
    """maby set when connecting with MA"""
    MA_AUTO = enum.auto()
    """set when MA auto mode enabled"""
    MA_CONNECTED = enum.auto()
    """set when MA connected"""
    UNKNOWN5 = enum.auto()
    """maby unused"""
    UNKNOWN6 = enum.auto()
    """maby unused"""
    UNKNOWN7 = enum.auto()
    """maby unused"""


class SetWorkModeCommand(_CommandO2):
    """
    Implements commands: `04`, `55`
    """

    # 00 default
    # 02
    # 03
    # 06 -> ma disconnected      | 0000 0110
    # 16 -> ma connected         | 0001 0110
    # after enabling ma pair
    # New work mode 0x06 -> 0x02 | 0000 0110 -> 0000 0010
    # New work mode 0x02 -> 0x03 | 0000 0010 -> 0000 0011
    # New work mode 0x03 -> 0x07 | 0000 0011 -> 0000 0111
    # New work mode 0x07 -> 0x03 | 0000 0111 -> 0000 0011
    # some time later
    # New work mode 0x03 -> 0x02 | 0000 0011 -> 0000 0010
    # New work mode 0x02 -> 0x12 | 0000 0010 -> 0001 0010
    # after disconnect from ma
    # New work mode 0x12 -> 0x02 | 0001 0010 -> 0000 0010
    state: WorkModeFlags = WorkModeFlags(0xFF)

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_set = CMD_SET_WORK_MODE_REQ
        self.cmd_rsp = CMD_SET_WORK_MODE_RSP

    def set(self, data: bytes) -> bytes:
        state = WorkModeFlags(data[0])
        if self.state != state:
            if self.state == 0xFF:
                self.state = WorkModeFlags(0)
            _LOGGER.info("New work mode %s -> %s (%d)", self.state, state, state)
        self.state = state
        return bytes()

    def upd(self, data: bytes) -> None:
        if len(data) == 1:
            self.state = data[0]

    def mk_set(self) -> tuple[CmdId, bytes]:
        return (self.cmd_set, bytes([self.state]))

    def __str__(self):
        state = self.state
        if state == 0xFF:
            state = WorkModeFlags(0)
        return f"work_mode={state}"


class DevInfoCommand(Command):
    """
    Implements commands: `07`, `17`

    Sample:
    ```txt
                           00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
      08a1:130c @ internet
      018f:130e @ denis    17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79
      ????:130c @ anatol   17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0C 13 04 10 EC 19 7B
    ```
    """

    sw_version: int = 0x130E
    hw_version: int = 0x6108
    heater_min: int = 0  # -20
    heater_max: int = 20  # 25

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_DEV_INFO_REQ
        self.cmd_rsp = CMD_DEV_INFO_RSP

    def get(self) -> bytes:
        return struct.pack(
            "<IIIIHHHbb",
            0x00000004,  # 04 00 00 00
            0x00000000,  # 00 00 00 00
            0x00000000,  # 00 00 00 00
            0x00000000,  # 00 00 00 00
            self.hw_version,  # 08 61
            self.sw_version,  # 0E 13
            0x1004,  # 04 10
            self.heater_min,  # EC
            self.heater_max,  # 19
        )

    def __str__(self) -> str:
        return f"dev_info[hw={self.hw_version}, sw={self.sw_version}, hmin={self.heater_min}, hmax={self.heater_max}]"

    def mk_req(self) -> tuple[CmdId, bytes]:
        return (self.cmd_req, bytes())


class ErrorFlags(_FlagsO2):
    EC01 = enum.auto()
    """Температура входящего воздуха более +50°C"""
    EC02 = enum.auto()
    """Температура уличного воздуха ниже значения, установленного параметром «Минимальная допустимая температура» (п.5.3.12)"""
    EC03 = enum.auto()
    """Температура выходящего воздуха более +50°C"""
    EC04 = enum.auto()
    """Значение температуры выходящего воздуха ниже 25°C (при включенном нагревателе)"""
    EC05 = enum.auto()
    """Ошибка работы заслонки"""
    EC06 = enum.auto()
    """Переохлаждение платы управления или индикации"""
    EC07 = enum.auto()
    """Ошибка измерения температуры"""
    EC08 = enum.auto()
    """Ошибка измерения температуры"""
    EC09 = enum.auto()
    """Ошибка измерения температуры"""
    EC10 = enum.auto()
    """Ошибка измерения температуры"""
    EC11 = enum.auto()
    """Ошибка измерения температуры"""
    UNKNOWN11 = enum.auto()
    UNKNOWN12 = enum.auto()
    UNKNOWN13 = enum.auto()
    UNKNOWN14 = enum.auto()
    UNKNOWN15 = enum.auto()


class StateFlags(_FlagsO2):
    FILTER = enum.auto()
    POWER = enum.auto()
    UNKNOWN2 = enum.auto()
    HEAT = enum.auto()
    UNKNOWN4 = enum.auto()
    UNKNOWN5 = enum.auto()
    UNKNOWN6 = enum.auto()
    UNKNOWN7 = enum.auto()


class StateCommand(_CommandO2):
    """
    Implements commands: `01`, `11`, `02`

    * 1: flags
      `1110`: heat=1, buzzer?=1, power=1, filter?=0
      `1100`: heat=1, buzzer?=1, power=0, filter?=0
      `0110`: heat=0, buzzer?=1, power=0, filter?=0
    * 2: outdoor_temp, signed
    * 3: current_temp, signed
    * 4: target_temp, signed, (-20:25)
    * 5: fan_speed, 1:4
    * 6: productivity (35,60,75,120)
    * 7: unknown7 всегда 04
    * 8,9: error 00 - не отшибок, 0x10 - ошибка EC05
    * 10,11,12,13: work_time in seconds
    * 14,15,16,17: filter_time in seconds
    """

    PRODUCTIVITY_MAP = {1: 34, 2: 59, 3: 74, 4: 119}

    flags: StateFlags = StateFlags(StateFlags.FILTER)
    outdoor_temp: int = 10
    current_temp: int = 10
    target_temp: int = 16
    fan_speed: int = 1
    productivity: int = PRODUCTIVITY_MAP[1]
    unknown7: int = 0x04
    error: int = 0x00  # 0x10 = EC05
    unknown9: int = 0x00
    work_time: int = 100 * 3600 * 24
    # filter_time: int = 30 * 3600 * 24
    filter_time = 0

    source: int = 1

    _STRUCT_GET = struct.Struct("<BbbbBBBBBII")
    _STRUCT_SET = struct.Struct("<Bb??B")

    def __init__(self, data: bytes = None):
        super().__init__(data)
        self.cmd_req = CMD_STATE_GET_REQ
        self.cmd_rsp = CMD_STATE_GET_RSP
        self.cmd_set = CMD_STATE_SET_REQ

    def get(self) -> bytes:
        start = datetime.datetime(year=2024, month=1, day=1)
        work_time = (datetime.datetime.now() - start).seconds
        self.work_time = work_time
        self.productivity = self.PRODUCTIVITY_MAP[self.fan_speed]
        # _LOGGER.info(
        #     "Get state: fan_speed=%d, target_temp=%d, power=%d, heat=%d",
        #     self.fan_speed,
        #     self.target_temp,
        #     self.power,
        #     self.heat,
        # )
        return self._STRUCT_GET.pack(
            self.flags,
            self.outdoor_temp,
            self.current_temp,
            self.target_temp,
            self.fan_speed,
            self.productivity,
            self.unknown7,
            self.error,
            self.unknown9,
            self.work_time,
            self.filter_time,
        )

    # 1: fan_speed, 1:4
    # 2: target_temp, signed -20:25
    # 3: power, 0 off, 1 on
    # 4: heat, 0 off, 1 on
    # 5: source, 0 ma_auto, 1 app
    def set(self, data: bytes) -> bytes:
        # 02 01 EC 01 01 01 11
        (
            self.fan_speed,
            self.target_temp,
            power,
            heat,
            self.source,
        ) = self._STRUCT_SET.unpack(data)
        self.flags = set_flag(self.flags, StateFlags.HEAT, heat)
        self.flags = set_flag(self.flags, StateFlags.POWER, power)

        _LOGGER.info(
            "Set state: fan=%d, T(t)=%d, flags=%s, src=%s",
            self.fan_speed,
            self.target_temp,
            self.flags,
            self._src,
        )
        return self.get()

    def upd(self, data: bytes) -> None:
        (
            flags,
            self.outdoor_temp,
            self.current_temp,
            self.target_temp,
            self.fan_speed,
            self.productivity,
            self.unknown7,
            self.error,
            self.unknown9,
            self.work_time,
            self.filter_time,
        ) = self._STRUCT_GET.unpack(data)
        self.flags = StateFlags(flags)

    def fan_up(self, up: bool = True):
        self.fan_speed = self._ctl_int("fan_speed", self.fan_speed, 1, 4, up)

    def power_up(self):
        self.flags = self._ctl_flag("power", self.flags, StateFlags.POWER)

    def heat_up(self):
        self.flags = self._ctl_flag("heat", self.flags, StateFlags.HEAT)

    def filter_up(self):
        self.flags = self._ctl_flag("filter", self.flags, StateFlags.FILTER)

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
        return (
            self.cmd_set,
            self._STRUCT_SET.pack(
                self.fan_speed,
                self.target_temp,
                get_flag(self.flags, StateFlags.POWER),
                get_flag(self.flags, StateFlags.HEAT),
                self.source,
            ),
        )

    @property
    def _src(self) -> str:
        if self.source == 1:
            return "USER"
        if self.source == 0:
            return "AUTO"
        return f"{self.source:02X}"

    def __str__(self):

        return (
            ""
            + f"(F)an={self.fan_speed}, "
            + f"T(c)={self.current_temp}, "
            + f"T(o)={self.outdoor_temp}, "
            + f"T(t)={self.target_temp}, "
            + f"flags={self.flags}, "
            + f"prod={self.productivity}, "
            + f"src={self._src},"
            + f"err={self.error:02X}"
        )
