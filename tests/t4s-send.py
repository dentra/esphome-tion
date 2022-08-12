#!/usr/bin/python3

import serial
import time
import sys
import struct
import binascii
import signal

CMD_HEARBEAT_REQ = 0x3932
CMD_HEARBEAT_RSP = 0x3931
CMD_STATE_SET = 0x3230
CMD_STATE_RSP = 0x3231
CMD_STATE_REQ = 0x3232
CMD_DEV_STATUS_REQ = 0x3332
CMD_DEV_STATUS_RSP = 0x3331
CMD_TIME_SET = 0x3630
CMD_TIME_REQ = 0x3632
CMD_TIME_RSP = 0x3631
CMD_TIMER_SET = 0x3430
CMD_TIMER_REQ = 0x3432
CMD_TIMER_RSP = 0x3431
CMD_TIMERS_STATE_REQ = 0x3532
CMD_TIMERS_STATE_RSP = 0x3531
CMD_TURBO_SET = 0x4130
CMD_TURBO_REQ = 0x4132
CMD_TURBO_RSP = 0x4131


FRAMES = {
    CMD_HEARBEAT_REQ: "HEARBEAT_REQ",
    CMD_HEARBEAT_RSP: "HEARBEAT_RSP",
    CMD_STATE_SET: "STATE_SET",
    CMD_STATE_RSP: "STATE_RSP",
    CMD_STATE_REQ: "STATE_REQ",
    CMD_DEV_STATUS_REQ: "DEV_STATUS_REQ",
    CMD_DEV_STATUS_RSP: "DEV_STATUS_RSP",
    CMD_TIME_SET: "TIME_SET",
    CMD_TIME_REQ: "TIME_REQ",
    CMD_TIME_RSP: "TIME_RSP",
    CMD_TIMER_SET: "TIMER_SET",
    CMD_TIMER_REQ: "TIMER_REQ",
    CMD_TIMER_RSP: "TIMER_RSP",
    CMD_TIMERS_STATE_REQ: "TIMERS_STATE_REQ",
    CMD_TIMERS_STATE_RSP: "TIMERS_STATE_RSP",
    CMD_TURBO_SET: "TURBO_SET",
    CMD_TURBO_REQ: "TURBO_REQ",
    CMD_TURBO_RSP: "TURBO_RSP",
}


class T4sSend:
    running = True

    def stop(self):
        self.running = False

    def dump(self, pos: str, buf: bytes):
        print(f"{pos} {buf.hex(' ').upper()}")
        (_, _, typ) = struct.unpack("<BHH", buf[:5])
        buf = buf[5:-2]
        print(f"{FRAMES.get(typ, f'{typ:04X}')}: {buf.hex('.').upper()} ({len(buf)})")

    def mk(self, x: bytes):
        if x[0] == 0x3A:
            return x
        l = len(x) + 5
        x = struct.pack("<BH", 0x3A, l) + x
        c = binascii.crc_hqx(x, 0xFFFF)
        x = x + struct.pack(">H", c)
        return x

    def start(self, s: bytes):
        self.running = True
        ser = serial.serial_for_url(
            "socket://tion4s-hw.local:8888", baudrate=9600, timeout=0.1
        )
        s = s.replace(" ", "")
        s = s.replace(".", "")
        x = bytes.fromhex(s)
        if len(x) > 0:
            x = self.mk(x)
            self.dump(">>>", x)
            ser.write(x)
            ser.flush()
            time.sleep(0.05)
        buf = bytes()
        while self.running:
            available = ser.in_waiting
            if available > 0:
                data = ser.read(available)
                if data[0] == 0x3A and len(buf) > 0:
                    break
                buf += data
        self.dump("<<<", buf)


def main(args: list[str]):
    t4s = T4sSend()

    def stop(signum, frame):  # pylint: disable=unused-argument
        print("\nStopping...")
        t4s.stop()

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    t4s.start("".join(args))


# main(sys.argv[1:])
