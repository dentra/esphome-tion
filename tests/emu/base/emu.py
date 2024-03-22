import asyncio
import logging
import queue
import threading
import time

from .device import Device
from .exchange import Exchange, Protocol
from .packet import Packet

_LOGGER = logging.getLogger()


class Emu:
    def __init__(self, name: str, exchange: Protocol, device: Device) -> None:
        self._device = device
        self.alive = threading.Event()
        self.reader = Exchange(name, exchange, self.alive)
        self.thread: threading.Thread = None

    @property
    def device(self):
        return self._device

    def __del__(self):
        self.stop()

    def stop(self):
        self.alive.set()

    def start(self) -> bool:
        if self.thread:
            _LOGGER.warning("emu already running")
            return False

        def loop_task():
            while not self.alive.is_set() and self.reader:
                self.loop()
                time.sleep(0.0005)

        self.reader.start()
        self.thread = threading.Thread(target=loop_task, daemon=True)
        self.thread.start()

        return True

    def loop(self):
        pkt = self.reader.read()
        if pkt:
            pkt = self.device.req(pkt)
        if pkt:
            self.reader.write(pkt)
