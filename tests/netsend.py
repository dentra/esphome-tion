import logging
import signal
import sys
import time
import serial

_LOGGER = logging.getLogger()


class NetSend:
    running = True

    def stop(self):
        self.running = False

    def start(self, data: str):
        ser_net = serial.serial_for_url("socket://tion4s-hw-connector.local:8888")
        data = data.replace(" ", "").replace(".", "")
        data = bytes.fromhex(data)
        print(f"Sending: {data.hex(' ')}")
        ser_net.write(data)
        while self.running:
            available = ser_net.in_waiting
            if available > 0:
                try:
                    print(ser_net.read(available))
                except serial.SerialException as err:
                    _LOGGER.error(err)
                    break
            time.sleep(1)


# 3A 07 00 32 33 6F A6
def main(argv: list[str]):
    netsend = NetSend()

    def stop(signum, frame):  # pylint: disable=unused-argument
        print("\nStopping...")
        netsend.stop()

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    netsend.start(argv[1])


main(sys.argv)
