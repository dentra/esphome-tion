import logging
import time
import serial

_LOGGER = logging.getLogger()


def main():
    ser_usb = serial.serial_for_url("/dev/cu.usbserial-A50285BI", baudrate=9600)
    ser_net = serial.serial_for_url("socket://tion4s-hw-connector.local:8888")
    while True:
        available = ser_usb.in_waiting
        if available > 0:
            try:
                ser_net.write(ser_usb.read(available))
            except serial.SerialException as err:
                _LOGGER.error(err)
                break
        available = ser_net.in_waiting
        if available > 0:
            try:
                ser_usb.write(ser_net.read(available))
            except serial.SerialException as err:
                _LOGGER.error(err)
                break
        time.sleep(1)


main()
