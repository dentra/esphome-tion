import abc
import queue
import threading
import time

import serial

from .command import CmdId
from .packet import Packet


class Transport(abc.ABC):

    @abc.abstractmethod
    def close(self):
        pass

    @property
    @abc.abstractmethod
    def available(self) -> int:
        pass

    @abc.abstractmethod
    def read(self, size: int = 1) -> bytes:
        pass

    @abc.abstractmethod
    def write(self, data: bytes) -> None:
        pass


class SerialTransport(Transport):
    def __init__(self, port, baudrate: int) -> None:
        self.serial = serial.serial_for_url(port, baudrate=baudrate, timeout=None)
        self.serial.flushInput()
        self.serial.flushOutput()
        time.sleep(0.1)  # wait for 100 ms for pyserial port to actually be ready

    @property
    def port(self):
        return self.serial.port

    @property
    def baudrate(self):
        return self.serial.baudrate

    @property
    def available(self) -> int:
        return self.serial.in_waiting

    def close(self):
        self.serial.close()

    def read(self, size: int = 1) -> bytes:
        return self.serial.read(size)

    def write(self, data: bytes) -> None:
        self.serial.write(data)
        self.serial.flush()


class Protocol(abc.ABC):
    def __init__(self, transport: Transport) -> None:
        self.transport = transport

    def close(self) -> None:
        self.transport.close()

    @abc.abstractmethod
    def rx(self) -> Packet:
        pass

    @abc.abstractmethod
    def tx(self, pkt: Packet) -> None:
        pass


class Exchange:
    def __init__(
        self, name: str, protocol: Protocol, alive: threading.Event = threading.Event()
    ) -> None:
        self.name = name
        self._protocol = protocol
        self._alive = alive
        self._rxq = queue.Queue()
        self._lock = threading.Lock()
        self._thread: threading.Thread = None

    def __del__(self):
        self.stop()

    def stop(self):
        self._alive.set()

    def start(self) -> bool:
        if self._thread:
            return False

        self._thread = threading.Thread(
            target=self._task, name=f"{self.name} thread", daemon=True
        )
        self._thread.start()

        return True

    def _task(self):
        while not self._alive.is_set():
            pkt = self._protocol.rx()
            if pkt:
                self._rxq.put(pkt)
            time.sleep(0.0005)

    def read(self) -> Packet | None:
        if self._rxq.empty():
            return None
        return self._rxq.get()

    def write(self, pkt: Packet | None):
        if pkt:
            with self._lock:
                self._protocol.tx(pkt)
