BLE_4S_SERVICE_NAME = "Breezer"
UUID_4S_SERVICE = "98f00001-3788-83ea-453e-f52244709ddb"
UUID_4S_CHAR_TX = "98f00002-3788-83ea-453e-f52244709ddb"
UUID_4S_CHAR_RX = "98f00003-3788-83ea-453e-f52244709ddb"

BLE_3S_SERVICE_NAME = "Breezer 3S"
UUID_3S_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
UUID_3S_CHAR_TX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
UUID_3S_CHAR_RX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLE_SERVICE_NAME = BLE_4S_SERVICE_NAME
UUID_SERVICE = UUID_4S_SERVICE
UUID_CHAR_TX = UUID_4S_CHAR_TX
UUID_CHAR_RX = UUID_4S_CHAR_RX

import asyncio
import logging
import sys
import threading
from typing import Any, Dict, Union

from bless import (  # type: ignore
    BlessGATTCharacteristic,
    BlessServer,
    GATTAttributePermissions,
    GATTCharacteristicProperties,
)

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(name=__name__)

# NOTE: Some systems require different synchronization methods.
trigger: Union[asyncio.Event, threading.Event]
if sys.platform in ["darwin", "win32"]:
    trigger = threading.Event()
else:
    trigger = asyncio.Event()


def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
    logger.debug(f"Reading {characteristic.value}")
    return characteristic.value


def write_request(
    characteristic: BlessGATTCharacteristic, value: Any, server: BlessServer, **kwargs
):
    print("wtite_char", characteristic)

    characteristic.value = value

    server.get_characteristic(UUID_CHAR_RX).value = value
    server.update_value(UUID_SERVICE, UUID_CHAR_RX)

    logger.debug(f"Char value set to {characteristic.value}")
    if characteristic.value == b"\x0f":
        logger.debug("NICE")
        trigger.set()


async def run(loop):
    trigger.clear()

    # Instantiate the server
    server = BlessServer(name=BLE_SERVICE_NAME, loop=loop)

    def write_req(char: BlessGATTCharacteristic, value: Any, **kwargs):
        write_request(char, value, server=server)

    server.read_request_func = read_request
    server.write_request_func = write_req

    # Add Service
    await server.add_new_service(UUID_SERVICE)
    # Add a Characteristic to the service
    await server.add_new_characteristic(
        UUID_SERVICE,
        UUID_CHAR_TX,
        GATTCharacteristicProperties.write,
        None,
        GATTAttributePermissions.readable | GATTAttributePermissions.writeable,
    )
    await server.add_new_characteristic(
        UUID_SERVICE,
        UUID_CHAR_RX,
        GATTCharacteristicProperties.notify,
        None,
        GATTAttributePermissions.readable | GATTAttributePermissions.writeable,
    )

    # logger.debug(server.get_characteristic(UUID_CHAR_TX))
    # logger.debug(server.get_characteristic(UUID_CHAR_RX))

    await server.start()
    # logger.debug("Advertising")
    # logger.info(f"Write '0xF' to the advertised characteristic: {UUID_CHAR_TX}")
    if trigger.__module__ == "threading":
        trigger.wait()
    else:
        await trigger.wait()

    await asyncio.sleep(2)
    logger.debug("Updating")
    server.get_characteristic(UUID_CHAR_TX)
    server.update_value(UUID_SERVICE, UUID_CHAR_TX)
    await asyncio.sleep(5)
    await server.stop()


loop = asyncio.get_event_loop()
loop.run_until_complete(run(loop))
