#!/usr/bin/env python3

import datetime
import logging
import os
import queue
import signal
import sys
import threading
import time

import blessed  # pip install blessed
import readchar  # pip install readchar

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from emu import o2

_LOGGER = logging.getLogger()

DISABLED_LOG = {}


def emu_show(term: blessed.Terminal, dev: o2.DeviceO2):
    sys.stdout.write(term.move(term.height - 0, 0))
    sys.stdout.write(term.clear_eol + str(dev.state))
    sys.stdout.write(term.move(term.height - 2, 0))
    sys.stdout.write(term.clear_eol + str(dev.dev_mode) + ", " + str(dev.work_mode))
    sys.stdout.write(term.csr(0, term.height - 3))
    sys.stdout.write(term.move(term.height - 3, 0))
    sys.stdout.flush()


def o2_ctl(key: str, dev: o2.DeviceO2, emu: o2.EmuO2Dev):
    def invert_work_mode_flag(flag: o2.WorkModeFlags):
        dev.work_mode.state = o2.set_flag(
            dev.work_mode.state, flag, not o2.get_flag(dev.work_mode.state, flag)
        )
        emu.write_work_mode()

    match key:
        case "t":
            dev.state.t_target_up(False)
            emu.write_state()
        case "T":
            dev.state.t_target_up(True)
            emu.write_state()
        case "f":
            dev.state.fan_up(True)
            emu.write_state()
        case "F":
            dev.state.fan_up(False)
            emu.write_state()
        case "p":
            dev.state.power_up()
            emu.write_state()
        case "h":
            dev.state.heat_up()
            emu.write_state()
        case "s":
            dev.state.source = 0 if dev.state.source == 1 else 0
            emu.write_state()
        case "m":
            invert_work_mode_flag(o2.WorkModeFlags.MA_CONNECTED)
        case "r":
            invert_work_mode_flag(o2.WorkModeFlags.RF_CONNECTED)
        case "0":
            invert_work_mode_flag(o2.WorkModeFlags.PAIR_ACCEPTED)
        case "2":
            invert_work_mode_flag(o2.WorkModeFlags.MA_PAIRING)
        case "3":
            invert_work_mode_flag(o2.WorkModeFlags.MA_AUTO)
        case "d":
            emu.request_dev_info()


def key_process(key: str, emu: o2.EmuO2Dev) -> bool:
    if key == "q":
        emu.stop()
        return True
    o2_ctl(key, emu.device, emu)
    return False


class QLogHandler(logging.Handler):

    def __init__(self, skip_modiles) -> None:
        super().__init__()
        self.q = queue.Queue()
        self.s = skip_modiles

    def emit(self, record: logging.LogRecord) -> None:
        try:
            self.q.put(record)
        except RecursionError:
            raise
        except Exception:
            self.handleError(record)

    def flush(self) -> None:
        if self.q.empty():
            return
        while not self.q.empty():
            record: logging.LogRecord = self.q.get()
            if record.module not in self.s:
                msg = self.format(record)
                sys.stdout.write(msg)
                sys.stdout.write("\n")
        sys.stdout.flush()


def setup_log(stop_event: threading.Event):
    fhandler = logging.FileHandler(os.path.basename(__file__) + ".log")
    fhandler.setLevel(logging.DEBUG)
    chandler = QLogHandler(["protocol"])
    chandler.setLevel(logging.INFO)
    logging.basicConfig(
        format="%(asctime)s %(module)s %(levelname)-7s %(message)s",
        level=logging.DEBUG,
        handlers=[fhandler, chandler],
    )

    log_thread = threading.Thread(
        target=log_flush_task, args=(chandler, stop_event), daemon=True
    )
    log_thread.start()


def log_flush_task(h: QLogHandler, stop: threading.Event):
    while not stop.is_set():
        h.flush()
        time.sleep(0.001)


def main(argv):
    if len(argv) != 2:
        _LOGGER.error("no port specified")
        return

    key_queue = queue.Queue()

    def keyboard_task(q: queue.Queue, stop: threading.Event):
        while not stop.is_set():
            q.put(readchar.readkey())
            time.sleep(0.1)

    stop_event = threading.Event()
    setup_log(stop_event)

    BAUD_RATE = 115200
    port_o2 = o2.SerialTransport(argv[1], BAUD_RATE)
    emu_dev = o2.EmuO2Dev(port_o2, disabled_log=DISABLED_LOG)

    key_thread = threading.Thread(
        target=keyboard_task, args=(key_queue, stop_event), daemon=True
    )
    key_thread.start()

    emu_dev.start()

    def stop(signum, frame):  # pylint: disable=unused-argument
        print("\nStopping...")
        stop_event.set()
        emu_dev.stop()
        time.sleep(1)
        exit(0)

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    # start = datetime.datetime(year=2024, month=1, day=1)
    # work_time = (datetime.datetime.now() - start).seconds

    last_time = None

    emu_dev.device.work_mode.state = 0

    term = blessed.Terminal()
    while True:
        emu_show(term, emu_dev.device)
        if not key_queue.empty() and key_process(key_queue.get(), emu_dev):
            stop_event.set()
            break

        if not last_time or (datetime.datetime.now() - last_time).seconds > 10:
            if not last_time:
                emu_dev.request_connect()
                time.sleep(0.01)
                emu_dev.request_dev_info()
            emu_dev.request_dev_mode()
            time.sleep(0.01)
            emu_dev.request_state()
            time.sleep(0.01)
            emu_dev.write_work_mode()

            last_time = datetime.datetime.now()

        emu_dev.write_work_mode()

        time.sleep(0.25)
    _LOGGER.info("Finished")


if __name__ == "__main__":
    main(sys.argv)
