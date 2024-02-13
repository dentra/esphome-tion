#!/usr/bin/env python3

# При активации RF модуля он выстаялет work_mode = 02[0000 0010]

# RF запрашивает состояние O2 по две (не считая дублей) команды 03 с интервалом 20мс между ними и переодичностью в 3сек:
# O2 new value DEV_MODE_RSP[13]: 02 [00000010]
# O2 new value DEV_MODE_RSP[13]: 00 [00000000]
# Активирован режим сопряжения с MA, опять ответы чередуются
# O2 new value DEV_MODE_RSP[13]: 03 [00000011]
# O2 new value DEV_MODE_RSP[13]: 00 [00000000]


## 17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0C 13 04 10 EC 19 7B
# unknown seqs
#    2E C1 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0C 13 04 10 EC 19 7B
# 00 1C D6 BE 38 04 00 00 D0 78 58 78 50 78 00 00 00 06 76 AF 88 00 00 00 00 00 00 00 00 D1 12 C7 06 C8 0B 11 85 7E (61)
# EC 2E C1 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0C 13 04 10 EC 19 7B
# 00 1C D6 BE 00 04 00 00 50 78 58 78 50 78 00 00 00 06 76 B0 00 00 00 00 00 00 00 00 00 CE 12 EB 06 98 16 80 CD 81 (62)

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

DISABLED_LOG = {
    # o2.CMD_CONNECT_REQ,
    # o2.CMD_CONNECT_RSP,
    # o2.CMD_DEV_INFO_REQ,
    # o2.CMD_DEV_INFO_RSP,
    # o2.CMD_DEV_MODE_REQ,
    # o2.CMD_DEV_MODE_RSP,
    # o2.CMD_SET_WORK_MODE_REQ,
    # o2.CMD_SET_WORK_MODE_RSP,
    # o2.CMD_STATE_GET_REQ,
    # o2.CMD_STATE_GET_RSP,
    # o2.CMD_STATE_SET_REQ,
    o2.CMD_TIME_GET_RSP,
}


def emu_show(term: blessed.Terminal, dev: o2.DeviceO2):
    sys.stdout.write(term.move(term.height - 0, 0))
    sys.stdout.write(term.clear_eol + str(dev.state))
    sys.stdout.write(term.move(term.height - 2, 0))
    sys.stdout.write(term.clear_eol + str(dev.dev_mode) + ", " + str(dev.work_mode))
    sys.stdout.write(term.csr(0, term.height - 3))
    sys.stdout.write(term.move(term.height - 3, 0))
    sys.stdout.flush()


def o2_ctl_proxy(key: str, dev: o2.DeviceO2, tion: o2.Exchange):
    match key:
        case "0":
            tion.write(o2.PacketO2.build_req(dev.connect))
        case "1":
            tion.write(o2.PacketO2.build_req(dev.state))
        case "2":
            tion.write(o2.PacketO2.build_set(dev.state))
        case "3":
            tion.write(o2.PacketO2.build_req(dev.dev_mode))
        case "4":
            tion.write(o2.PacketO2.build_set(dev.work_mode))
        case "5":
            tion.write(o2.PacketO2.build_req(dev.time))
        case "7":
            tion.write(o2.PacketO2.build_req(dev.dev_info))


def o2_ctl(key: str, dev: o2.DeviceO2, emu: o2.Emu):
    if isinstance(emu, o2.EmuO2Proxy):
        o2_ctl_proxy(key, dev, emu.o2)

    match key:
        case "o":
            dev.state.t_outdoor_up(False)
        case "O":
            dev.state.t_outdoor_up(True)
        case "t":
            dev.state.t_target_up(False)
        case "T":
            dev.state.t_target_up(True)
        case "c":
            dev.state.t_current_up(False)
        case "C":
            dev.state.t_current_up(True)
        case "f":
            dev.state.fan_up(True)
        case "F":
            dev.state.fan_up(False)
        case "p":
            dev.state.power_up()
        case "h":
            dev.state.heat_up()
        case "r":
            dev.state.filter_up()
        case "b":
            dev.dev_mode.switch_user()
        case "m":
            dev.dev_mode.switch_pair()


def key_process(key: str, emu: o2.Emu) -> bool:
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
    key_queue = queue.Queue()

    def keyboard_task(q: queue.Queue, stop: threading.Event):
        while not stop.is_set():
            q.put(readchar.readkey())
            time.sleep(0.1)

    stop_event = threading.Event()
    setup_log(stop_event)

    port_rf = o2.SerialTransport("loop://?logging=info", 115200)
    if len(argv) > 1:
        port_rf = o2.SerialTransport(argv[1], 115200)
    port_o2 = None
    if len(argv) > 2:
        port_o2 = o2.SerialTransport(argv[2], 115200)

    emu: o2.EmuO2
    if port_o2:
        _LOGGER.info("Proxy mode enabled: %s", port_o2)
        emu = o2.EmuO2Proxy(port_rf, port_o2, disabled_log=DISABLED_LOG)
    else:
        emu = o2.EmuO2(port_rf, disabled_log=DISABLED_LOG)

    key_thread = threading.Thread(
        target=keyboard_task, args=(key_queue, stop_event), daemon=True
    )
    key_thread.start()

    emu.start()

    def stop(signum, frame):  # pylint: disable=unused-argument
        print("\nStopping...")
        stop_event.set()
        emu.stop()
        time.sleep(1)
        exit(0)

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    term = blessed.Terminal()
    while True:
        emu_show(term, emu.device)
        if not key_queue.empty() and key_process(key_queue.get(), emu):
            stop_event.set()
            break
        time.sleep(0.25)
    _LOGGER.info("Finished")


if __name__ == "__main__":
    main(sys.argv)
