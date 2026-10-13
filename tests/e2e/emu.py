"""Breezer emulators connected to firmware UART through pty."""

import asyncio
import logging
import os
import pty
import struct
import tty
from dataclasses import dataclass

_LOGGER = logging.getLogger(__name__)


def crc16(data: bytes) -> int:
    """CRC-16/CCITT-FALSE."""
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


class PtyPort:
    """Pseudo terminal exposed to firmware by symlink path."""

    def __init__(self, link: str) -> None:
        self.link = link
        self.master, self.slave = pty.openpty()
        tty.setraw(self.master)
        tty.setraw(self.slave)
        if os.path.lexists(link):
            os.unlink(link)
        os.symlink(os.ttyname(self.slave), link)

    def write(self, data: bytes) -> None:
        os.write(self.master, data)

    def close(self) -> None:
        if os.path.lexists(self.link):
            os.unlink(self.link)
        os.close(self.master)
        os.close(self.slave)


@dataclass
class Frame:
    type: int
    data: bytes


class UartEmulator:
    """Base emulator: reads firmware output from pty and keeps received frames."""

    def __init__(self, port: PtyPort) -> None:
        self.port = port
        # all received frames
        self.frames: list[Frame] = []
        # do not answer when False
        self.online = True
        self._rx = bytearray()

    def attach(self, loop: asyncio.AbstractEventLoop) -> None:
        loop.add_reader(self.port.master, self._on_read)

    def detach(self, loop: asyncio.AbstractEventLoop) -> None:
        loop.remove_reader(self.port.master)

    def received(self, frame_type) -> list[Frame]:
        return [f for f in self.frames if f.type == frame_type]

    def _on_read(self) -> None:
        try:
            self._rx += os.read(self.port.master, 1024)
        except OSError:
            return
        for frame in self._parse():
            self.frames.append(frame)
            if self.online:
                self._handle(frame)

    def _parse(self) -> list[Frame]:
        """Extracts complete frames from self._rx."""
        self._rx.clear()
        return []

    def _handle(self, frame: Frame) -> None:
        pass


class SilentBreezer(UartEmulator):
    """Reads and drops everything, otherwise firmware blocks on full pty buffer."""


#
# Tion 4S
#


@dataclass
class Tion4sState:
    power_state: bool = True
    sound_state: bool = True
    led_state: bool = True
    # heater is enabled by user (heater mode)
    heater_state: bool = False
    # heater is heating now
    heating: bool = False
    gate_position: int = 0
    target_temperature: int = 16
    fan_speed: int = 2
    outdoor_temperature: int = 10
    current_temperature: int = 18
    pcb_ctl_temperature: int = 20
    pcb_pwr_temperature: int = 25
    work_time: int = 100000
    fan_time: int = 90000
    filter_time: int = 15552000
    airflow_counter: int = 1000
    errors: int = 0
    max_fan_speed: int = 6
    heater_var: int = 0
    # auto mode is reported by breezer as ma_connected
    ma_connected: bool = False

    def pack(self) -> bytes:
        flags = (
            (self.power_state << 0)
            | (self.sound_state << 1)
            | (self.led_state << 2)
            | (self.heating << 3)
            # heater mode: 0 - heating, 1 - fan only
            | ((not self.heater_state) << 4)
            # heater present 1400W
            | (2 << 7)
            | (self.ma_connected << 10)
        )
        return struct.pack(
            "<HBbBbbbbIIIIIBB",
            flags,
            self.gate_position,
            self.target_temperature,
            self.fan_speed,
            self.outdoor_temperature,
            self.current_temperature,
            self.pcb_ctl_temperature,
            self.pcb_pwr_temperature,
            self.work_time,
            self.fan_time,
            self.filter_time,
            self.airflow_counter,
            self.errors,
            self.max_fan_speed,
            self.heater_var,
        )

    def apply_set(self, data: bytes) -> None:
        flags, ext_flags, gate, target, fan, _filter_time = struct.unpack("<BBBbBH", data)
        self.power_state = bool(flags & (1 << 0))
        self.sound_state = bool(flags & (1 << 1))
        self.led_state = bool(flags & (1 << 2))
        self.heater_state = not flags & (1 << 3)
        self.gate_position = gate
        self.target_temperature = target
        self.fan_speed = fan
        self.ma_connected = bool(ext_flags & (1 << 0))


class Tion4sUart(UartEmulator):
    """Tion 4S UART: 3A <size LE16> <type LE16> <data> <crc16 BE>."""

    MAGIC = 0x3A
    STATE_SET = 0x3230
    STATE_RSP = 0x3231
    STATE_REQ = 0x3232
    DEV_INFO_RSP = 0x3331
    DEV_INFO_REQ = 0x3332
    TIME_SET = 0x3630
    TIME_RSP = 0x3631
    TIME_REQ = 0x3632
    HEARTBEAT_RSP = 0x3931
    HEARTBEAT_REQ = 0x3932

    def __init__(self, port: PtyPort) -> None:
        super().__init__(port)
        self.state = Tion4sState()
        self.firmware_version = 0x0230

    def _parse(self) -> list[Frame]:
        frames = []
        while True:
            start = self._rx.find(self.MAGIC)
            if start < 0:
                self._rx.clear()
                return frames
            del self._rx[:start]
            if len(self._rx) < 3:
                return frames
            size = self._rx[1] | self._rx[2] << 8
            if size < 7 or size > 64:
                del self._rx[:1]
                continue
            if len(self._rx) < size:
                return frames
            raw = bytes(self._rx[:size])
            del self._rx[:size]
            if crc16(raw) != 0:
                _LOGGER.warning("4s: bad crc %s", raw.hex("."))
                continue
            frames.append(Frame(raw[3] | raw[4] << 8, raw[5:-2]))

    def _send(self, frame_type: int, data: bytes) -> None:
        raw = struct.pack("<BHH", self.MAGIC, len(data) + 7, frame_type) + data
        self.port.write(raw + struct.pack(">H", crc16(raw)))

    def send_state(self, request_id: int = 1) -> None:
        self._send(self.STATE_RSP, struct.pack("<I", request_id) + self.state.pack())

    def _handle(self, frame: Frame) -> None:
        if frame.type == self.HEARTBEAT_REQ:
            self._send(self.HEARTBEAT_RSP, b"\x01")
        elif frame.type == self.DEV_INFO_REQ:
            info = struct.pack("<BIHH", 1, 0x8003, self.firmware_version, 0x0100) + bytes(16)
            self._send(self.DEV_INFO_RSP, info)
        elif frame.type == self.STATE_REQ:
            self.send_state(1)
        elif frame.type == self.STATE_SET:
            (request_id,) = struct.unpack("<I", frame.data[:4])
            self.state.apply_set(frame.data[4:])
            self.send_state(request_id)
        elif frame.type == self.TIME_REQ:
            (request_id,) = struct.unpack("<I", frame.data[:4])
            self._send(self.TIME_RSP, struct.pack("<IQ", request_id, 0))
        else:
            _LOGGER.debug("4s: unhandled frame %04X %s", frame.type, frame.data.hex("."))


#
# Tion 3S
#


@dataclass
class Tion3sState:
    power_state: bool = True
    heater_state: bool = False
    sound_state: bool = True
    # 0 - indoor, 1 - mixed, 2 - outdoor
    gate_position: int = 2
    target_temperature: int = 16
    fan_speed: int = 2
    current_temperature: int = 18
    outdoor_temperature: int = 10
    filter_days_left: int = 100
    last_error: int = 0
    productivity: int = 60
    firmware_version: int = 0x003C
    ma_auto: bool = False
    # filter reset requests
    filter_resets: int = 0

    def pack(self) -> bytes:
        flags = (
            (self.heater_state << 0)
            | (self.power_state << 1)
            | (self.sound_state << 3)
            | (self.ma_auto << 4)
            | (self.ma_auto << 5)
            # preset state, always set by Tion Remote
            | (1 << 8)
        )
        return struct.pack(
            "<BbHbbbHBBBBHH",
            (self.fan_speed & 0x0F) | (self.gate_position << 4),
            self.target_temperature,
            flags,
            self.current_temperature,
            self.current_temperature,
            self.outdoor_temperature,
            self.filter_days_left,
            12,
            30,
            self.last_error,
            self.productivity,
            0,
            self.firmware_version,
        )

    def apply_set(self, data: bytes) -> None:
        fan, target, gate, flags, filter_flags, _filter_value = struct.unpack("<BbBHBH", data[:8])
        self.fan_speed = fan
        self.target_temperature = target
        self.gate_position = gate
        self.heater_state = bool(flags & (1 << 0))
        self.power_state = bool(flags & (1 << 1))
        self.sound_state = bool(flags & (1 << 3))
        self.ma_auto = bool(flags & (1 << 4))
        if filter_flags & (1 << 1):
            self.filter_resets += 1
            self.filter_days_left = 180


class Tion3sUart(UartEmulator):
    """Tion 3S UART: 20 bytes frames, request 3D <cmd> <17 bytes> 5A, response B3 <cmd << 4> <17 bytes> 5A."""

    REQ = 0x3D
    RSP = 0xB3
    END = 0x5A
    FRAME_SIZE = 20
    STATE_GET = 0x01
    STATE_SET = 0x02

    def __init__(self, port: PtyPort) -> None:
        super().__init__(port)
        self.state = Tion3sState()

    def _parse(self) -> list[Frame]:
        frames = []
        while True:
            start = self._rx.find(self.REQ)
            if start < 0:
                self._rx.clear()
                return frames
            del self._rx[:start]
            if len(self._rx) < self.FRAME_SIZE:
                return frames
            raw = bytes(self._rx[: self.FRAME_SIZE])
            if raw[-1] != self.END:
                del self._rx[:1]
                continue
            del self._rx[: self.FRAME_SIZE]
            frames.append(Frame(raw[1], raw[2:-1]))

    def send_state(self) -> None:
        self.port.write(bytes([self.RSP, self.STATE_GET << 4]) + self.state.pack() + bytes([self.END]))

    def _handle(self, frame: Frame) -> None:
        if frame.type == self.STATE_GET:
            self.send_state()
        elif frame.type == self.STATE_SET:
            self.state.apply_set(frame.data)
            self.send_state()
        else:
            _LOGGER.debug("3s: unhandled frame %02X %s", frame.type, frame.data.hex("."))


#
# Tion O2
#


def crc_o2(data: bytes) -> int:
    crc = 0xFF
    for b in data:
        crc ^= b
    return crc


@dataclass
class TionO2State:
    power_state: bool = True
    heater_state: bool = False
    filter_state: bool = False
    outdoor_temperature: int = 10
    current_temperature: int = 18
    target_temperature: int = 16
    fan_speed: int = 2
    productivity: int = 60
    errors: int = 0
    work_time: int = 31250100
    filter_time: int = 30078465

    def pack(self) -> bytes:
        flags = (self.filter_state << 0) | (self.power_state << 1) | (1 << 2) | (self.heater_state << 3)
        return struct.pack(
            "<BbbbBBBHII",
            flags,
            self.outdoor_temperature,
            self.current_temperature,
            self.target_temperature,
            self.fan_speed,
            self.productivity,
            4,
            self.errors,
            self.work_time,
            self.filter_time,
        )

    def apply_set(self, data: bytes) -> None:
        fan, target, power, heater, _comm = struct.unpack("<BbBBB", data)
        self.fan_speed = fan
        self.target_temperature = target
        self.power_state = bool(power)
        self.heater_state = bool(heater)


class TionO2Uart(UartEmulator):
    """Tion O2 UART: <type> <data> <xor crc>, frame size depends on type."""

    CONNECT_REQ = 0x00
    STATE_GET_REQ = 0x01
    STATE_SET_REQ = 0x02
    DEV_MODE_REQ = 0x03
    SET_WORK_MODE_REQ = 0x04
    TIME_GET_REQ = 0x05
    TIME_SET_REQ = 0x06
    DEV_INFO_REQ = 0x07
    STATE_GET_RSP = 0x11
    # data size with crc
    REQ_SIZES = {
        CONNECT_REQ: 1,
        STATE_GET_REQ: 1,
        STATE_SET_REQ: 6,
        DEV_MODE_REQ: 1,
        SET_WORK_MODE_REQ: 2,
        TIME_GET_REQ: 1,
        TIME_SET_REQ: 3,
        DEV_INFO_REQ: 1,
    }
    # real responses
    CONNECT_RSP = bytes.fromhex("10 04 10 01 00 FA")
    DEV_MODE_RSP = bytes.fromhex("13 00 EC")
    WORK_MODE_RSP = bytes.fromhex("55 AA")
    TIME_RSP = bytes.fromhex("15 16 34 09 C1")
    DEV_INFO_RSP = bytes.fromhex("17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79")

    def __init__(self, port: PtyPort) -> None:
        super().__init__(port)
        self.state = TionO2State()

    def _parse(self) -> list[Frame]:
        frames = []
        while self._rx:
            size = self.REQ_SIZES.get(self._rx[0])
            if size is None:
                _LOGGER.warning("o2: unknown frame %s", self._rx.hex("."))
                self._rx.clear()
                return frames
            if len(self._rx) < size + 1:
                return frames
            raw = bytes(self._rx[: size + 1])
            del self._rx[: size + 1]
            if crc_o2(raw) != 0:
                _LOGGER.warning("o2: bad crc %s", raw.hex("."))
                continue
            frames.append(Frame(raw[0], raw[1:-1]))
        return frames

    def send_state(self) -> None:
        data = bytes([self.STATE_GET_RSP]) + self.state.pack()
        self.port.write(data + bytes([crc_o2(data)]))

    def _handle(self, frame: Frame) -> None:
        if frame.type == self.CONNECT_REQ:
            self.port.write(self.CONNECT_RSP)
        elif frame.type == self.DEV_INFO_REQ:
            self.port.write(self.DEV_INFO_RSP)
        elif frame.type == self.DEV_MODE_REQ:
            self.port.write(self.DEV_MODE_RSP)
        elif frame.type == self.SET_WORK_MODE_REQ:
            self.port.write(self.WORK_MODE_RSP)
        elif frame.type == self.TIME_GET_REQ:
            self.port.write(self.TIME_RSP)
        elif frame.type == self.STATE_GET_REQ:
            self.send_state()
        elif frame.type == self.STATE_SET_REQ:
            self.state.apply_set(frame.data)
            self.send_state()


#
# Tion Lite
#


@dataclass
class TionLtState:
    power_state: bool = True
    heater_state: bool = False
    fan_speed: int = 2
    target_temperature: int = 16
    outdoor_temperature: int = 10
    current_temperature: int = 18
    heater_var: int = 0
    filter_time: int = 10984449
    fan_time: int = 27853548
    work_time: int = 81257423
    errors: int = 0
    firmware_version: int = 0x054B
    sound_state: bool = True
    led_state: bool = True


class TionLtUart(UartEmulator):
    """Tion Lite UART: text console, commands end with CRLF."""

    def __init__(self, port: PtyPort) -> None:
        super().__init__(port)
        self.state = TionLtState()

    def _parse(self) -> list[Frame]:
        frames = []
        while (end := self._rx.find(b"\r\n")) >= 0:
            line = bytes(self._rx[:end]).decode(errors="replace")
            del self._rx[: end + 2]
            if line:
                cmd, _, arg = line.partition(" ")
                frames.append(Frame(cmd, arg.encode()))
        return frames

    def send_state(self) -> None:
        st = self.state
        text = (
            "\r\n"
            "Switching Mode\r\n"
            f"Current Mode: {'Work' if st.power_state else 'StandBy'}\r\n"
            f"Speed: {st.fan_speed}\r\n"
            f"Sensors T_set: {st.target_temperature}, T_In: {st.outdoor_temperature}, "
            f"T_out: {st.current_temperature}\r\n"
            f"PID_Value: {st.heater_var} {int(st.heater_state)}\r\n"
            f"Filter Time: {st.filter_time}\r\n"
            f"Working Time: {st.fan_time}\r\n"
            f"Power On Time: {st.work_time}\r\n"
            f"Error register: {st.errors}\r\n"
            "MAC: 247 74 249 223 116 211\r\n"
            f"Firmware Version 0x{st.firmware_version:04X}\r\n"
            "\r\n"
        )
        self.port.write(text.encode())

    def _handle(self, frame: Frame) -> None:
        st = self.state
        arg = frame.data.decode()
        if frame.type == "getstate":
            self.send_state()
        elif frame.type == "set_speed":
            st.fan_speed = int(arg)
        elif frame.type == "pon":
            st.power_state = True
            # zero speed is changed to 1 on power on
            st.fan_speed = st.fan_speed or 1
        elif frame.type == "stby":
            st.power_state = False
        elif frame.type == "set_heater_state":
            st.heater_state = arg == "1"
        elif frame.type == "set_temp":
            st.target_temperature = int(arg)
        elif frame.type == "set_sound_state":
            st.sound_state = arg == "1"
        elif frame.type == "set_led_state":
            st.led_state = arg == "1"
        elif frame.type == "ftreset":
            st.filter_time = 15552000
        elif frame.type == "set_filtertime":
            st.filter_time = int(arg)
