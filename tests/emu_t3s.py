#!/usr/bin/env python3
# pylint: disable=missing-module-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring
# import time
import abc

# from urllib import request
# import yaml
import binascii
import datetime
import logging
import signal
import struct
import sys
import time
from typing import Callable

import serial

_LOGGER = logging.getLogger()

ONOFF = ["OFF", "ON"]
FAN_SPEED = ["OFF", "1", "2", "3", "4", "5", "6"]
GATE_POS = ["INFLOW", "RECIRCULATION"]


class Dongle:
    @abc.abstractmethod
    def write_cmd(self, cmd: int, data: str):
        pass


class Device:
    _reqs: dict[int, Callable] = {}

    def __init__(self, dongle: Dongle):
        self._dongle = dongle

    @abc.abstractmethod
    def get_name(self):
        pass

    def _write_cmd(self, cmd: int, data: bytes):
        self._dongle.write_cmd(cmd, data)

    def process_cmd(self, cmd: int, buf: bytes) -> bool:
        if cmd in self._reqs:
            self._reqs[cmd](buf)
            return True
        return False


class Tion4sState:
    power_state: bool = True
    sound_state: bool = True
    led_state: bool = True
    heater_state: bool = True
    gate_pos: int = 0
    target_temp: int = 16
    fan_speed: int = 1

    def dump(self):
        _LOGGER.info(
            "power: %s, sound: %s, led: %s, heater: %s, gate: %s, target_temp: %d°C, fan_speed: %s",
            ONOFF[self.power_state],
            ONOFF[self.sound_state],
            ONOFF[self.led_state],
            ONOFF[self.heater_state],
            GATE_POS[self.gate_pos],
            self.target_temp,
            FAN_SPEED[self.fan_speed],
        )

    def pack(self, request_id) -> bytes:
        flags = 0
        if self.power_state:
            flags |= 1 << 0
        if self.sound_state:
            flags |= 1 << 1
        if self.led_state:
            flags |= 1 << 2
        if self.heater_state:
            flags |= 1 << 3
        else:
            flags |= 1 << 4  # heater_mode
        return struct.pack(
            "<IHBbBbbbbIIIIIBB",
            request_id,
            flags,
            self.gate_pos,
            self.target_temp,
            self.fan_speed,
            18,  # outdoor temp
            20,  # current temp
            self.target_temp,
            self.target_temp,
            1,  # cnt work_time
            0,  # cnt fan_time
            0,  # cnt filter_time
            0,  # cnt airflow_counter
            0,  # errors
            6,  # max fan
            0,  # filter rime
        )

    def unpack(self, buf: bytes) -> int:
        fmt16 = "<IHBbBH"
        fmt32 = "<IHBbBI"
        (
            request_id,
            flags,
            self.gate_pos,
            self.target_temp,
            self.fan_speed,
            _,
        ) = struct.unpack(fmt16 if struct.calcsize(fmt16) == len(buf) else fmt32, buf)
        self.power_state = (flags & (1 << 0)) != 0
        self.sound_state = (flags & (1 << 1)) != 0
        self.led_state = (flags & (1 << 2)) != 0
        self.heater_state = (flags & (1 << 3)) != 0
        return request_id


class Tion4s(Device):
    CMD_HEARBEAT_REQ = 0x3932
    CMD_HEARBEAT_RSP = 0x3931
    CMD_STATE_SET = 0x3230
    CMD_STATE_RSP = 0x3231
    CMD_STATE_REQ = 0x3232
    CMD_DEV_INFO_REQ = 0x3332
    CMD_DEV_INFO_RSP = 0x3331
    CMD_TIME_SET = 0x3630
    CMD_TIME_REQ = 0x3632
    CMD_TIME_RSP = 0x3631

    state = Tion4sState()

    def __init__(self, dongle: Dongle):
        super().__init__(dongle)
        self._reqs[self.CMD_HEARBEAT_REQ] = self.heartbeat_req
        self._reqs[self.CMD_STATE_REQ] = self.state_req
        self._reqs[self.CMD_STATE_SET] = self.state_set
        self._reqs[self.CMD_DEV_INFO_REQ] = self.dev_info_req
        self._reqs[self.CMD_TIME_SET] = self.time_set
        self._reqs[self.CMD_TIME_REQ] = self.time_req

    def get_name(self):
        return "Tion 4S"

    def heartbeat_rsp(self) -> None:
        self._write_cmd(self.CMD_HEARBEAT_RSP, b"\x01")

    def heartbeat_req(self, buf: bytes) -> None:  # pylint: disable=unused-argument
        _LOGGER.debug("heartbeat_req: %s", buf.hex(" "))
        self.heartbeat_rsp()

    def state_rsp(self, request_id) -> None:
        self.state.dump()
        self._write_cmd(self.CMD_STATE_RSP, self.state.pack(request_id))

    def state_req(self, buf: bytes) -> None:  # pylint: disable=unused-argument
        _LOGGER.debug("state_req: %s", buf.hex(" "))
        request_id = struct.unpack("<I", buf[:4])[0] if len(buf) >= 4 else 0
        self.state_rsp(request_id)

    def state_set(self, buf: bytes) -> None:
        _LOGGER.debug("state_set: %s", buf.hex(" "))
        self.state_rsp(self.state.unpack(buf))

    def dev_info_rsp(self) -> None:
        data = struct.pack("<BHHHH", 0x01, 0x8003, 0, 0x02D2, 0x01) + b"\x00" * 16
        self._write_cmd(self.CMD_DEV_INFO_RSP, data)

    def dev_info_req(self, buf: bytes) -> None:
        _LOGGER.debug("dev_info: %s", buf.hex(" "))
        self.dev_info_rsp()

    def time_rsp(self, request_id: int) -> None:
        now = datetime.datetime.now().replace(tzinfo=datetime.timezone.utc).timestamp()
        self._write_cmd(self.CMD_TIME_RSP, struct.pack("<IQ", request_id, int(now)))

    def time_req(self, buf: bytes) -> None:
        _LOGGER.debug("time_req: %s", buf.hex(" "))
        request_id = struct.unpack("<I", buf[:4])[0] if len(buf) >= 4 else 0
        self.time_rsp(request_id)

    def time_set(self, buf: bytes) -> None:
        _LOGGER.debug("time_set: %s", buf.hex(" "))
        (request_id, unix_time) = struct.unpack("<IQ", buf)
        _LOGGER.info(
            "time_set: request_id: %u, time: %s",
            request_id,
            datetime.datetime.utcfromtimestamp(unix_time).replace(
                tzinfo=datetime.datetime.now().astimezone().tzinfo
            ),
        )
        self.time_rsp(request_id)


class DongleEmu(Dongle):
    def __init__(self, url: str):
        super().__init__()
        self._device = Tion4s(self)
        self._ser = serial.serial_for_url(url, baudrate=115200)
        self._ser.flushInput()
        self._ser.flushOutput()
        self._running = False

    def __del__(self):
        self.close()

    def close(self):
        if self._ser is not None:
            self._ser.close()

    def stop(self):
        self._running = False

    def run(self):
        time.sleep(0.1)  # wait for 100 ms for pyserial port to actually be ready
        self._running = True
        buf = bytes()
        while self._running:
            available = self._ser.in_waiting
            if available > 0:
                try:
                    buf = self._rx_buf(buf + self._ser.read(available))
                except serial.SerialException as err:
                    _LOGGER.error(err)
                    break
            time.sleep(1)

    def _rx_buf(self, buf: bytes) -> bytes:
        """Process input buffer"""
        _LOGGER.debug("RX: %s (%d)", buf.hex(" ").upper(), len(buf))
        max_buf_len = 0x2A
        unk = bytearray()
        while len(buf) > 0:
            # _LOGGER.debug("buf: %s (%d)", buf.hex(" ").upper(), len(buf))
            if buf[0] == 0x3A:
                if len(buf) < 3:
                    break  # not enought data len
                end = buf[2] << 8 | buf[1]
                _LOGGER.debug("packet len: %d", end)
                if end > max_buf_len:
                    _LOGGER.warning("packet len is too long: %d", end)
                    unk.append(buf[0])
                    buf = buf[1:]
                    # sys.exit(1)
                    continue
                if len(buf) < end:
                    break  # not enought data len
                self._rx_packet(buf[0:end])
                buf = buf[end:]
                continue

            unk.append(buf[0])
            buf = buf[1:]
        if len(unk) > 0:
            _LOGGER.debug("Unknown bytes: %s (%d)", unk.hex(" ").upper(), len(unk))
        return buf

    def _rx_packet(self, buf: bytes) -> None:
        _LOGGER.info("RX: %s (%d)", buf.hex(" ").upper(), len(buf))
        if binascii.crc_hqx(buf, 0xFFFF) != 0:
            _LOGGER.warning("Invalid CRC")
            return
        cmd = buf[4] << 8 | buf[3]
        buf = buf[5 : len(buf) - 2]
        if not self._device.process_cmd(cmd, buf):
            _LOGGER.warning(
                "Unknown packet command: %02X, data: %s (%d)",
                cmd,
                buf.hex("."),
                len(buf),
            )

    def write_cmd(self, cmd: int, data: bytes):
        """Write command"""
        buf = struct.pack("<bHH", 0x3A, len(data) + 7, cmd) + data
        crc = binascii.crc_hqx(buf, 0xFFFF)
        buf = buf + struct.pack(">H", crc)
        _LOGGER.info("TX: %s (%d)", buf.hex(" ").upper(), len(buf))
        if not self.is_test():
            self._ser.write(buf)
            self._ser.flush()
            time.sleep(0.1)

    def test(self, data: str):
        if self.is_test():
            data = data.replace(" ", "").replace(".", "")
            self._ser.write(bytes.fromhex(data))

    def is_test(self) -> bool:
        return self._ser.port.startswith("loop://")


def main(argv: list[str]):
    logging.basicConfig(format="%(asctime)s %(levelname)-7s %(message)s")
    _LOGGER.setLevel(logging.DEBUG)
    emu = DongleEmu(argv[1] if len(argv) == 2 else "loop://?logging=info")

    def stop(signum, frame):  # pylint: disable=unused-argument
        print("\nStopping...")
        emu.stop()

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    if emu.is_test():
        emu.test("3A 0700 3239 CEEC")
        emu.test("3A 0700 3233 6FA6")
        emu.test("3a 1200 3032 02000000 0600 00 0c 03 a4c5 76ca")
        emu.test(
            "3a 1200 3032 02000000 0600 00 0c 03 a4c5 76ca3A 0700 3239 CEEC3a 1200 3032 02000000 0600 00 0c 03 a4c5 76ca"
        )
        emu.test(
            "3A 20 00 31 33 01 03 80 00 00 D2 02 D2 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 20 64"
        )

    emu.run()


def ttt(buf: bytes):
    (request_id, unix_time) = struct.unpack("<IQ", buf)
    _LOGGER.info(
        "time_set: request_id: %u, time: %s",
        request_id,
        datetime.datetime.utcfromtimestamp(unix_time).replace(
            tzinfo=datetime.datetime.now().astimezone().tzinfo
        ),
    )
    _LOGGER.info(
        "time_set: request_id: %u, time: %s",
        request_id,
        datetime.datetime.utcfromtimestamp(unix_time / 1000000).replace(
            tzinfo=datetime.datetime.now().astimezone().tzinfo
        ),
    )

    (request_id, unix_time) = struct.unpack("<II", buf[:-4])
    _LOGGER.info(
        "time_set: request_id: %u, time: %s",
        request_id,
        datetime.datetime.utcfromtimestamp(unix_time).replace(
            tzinfo=datetime.datetime.now().astimezone().tzinfo
        ),
    )


def tt():
    logging.basicConfig(format="%(asctime)s %(levelname)-7s %(message)s")
    _LOGGER.setLevel(logging.DEBUG)
    ttt(bytes.fromhex("00.00.00.00.e5.77.e2.62.00.00.00.00".replace(".", "")))
    ttt(
        struct.pack(
            "<IQ",
            0,
            int(
                datetime.datetime.now()
                .replace(tzinfo=datetime.timezone.utc)
                .timestamp()
            ),
        )
    )


def t2():
    flags = 0x17
    print("%02X" % flags, bin(flags))
    print((flags & (1 << 0)) != 0)
    print((flags & (1 << 1)) != 0)
    print((flags & (1 << 2)) != 0)


if __name__ == "__main__":
    main(sys.argv)
    # tt()
