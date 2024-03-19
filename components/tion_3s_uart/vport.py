import esphome.codegen as cg
from esphome.core import CORE

from .. import tion, vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport", "tion"]

Tion3sUartVPort = tion.tion_ns.class_(
    "Tion3sUartVPort", cg.PollingComponent, vport.VPort
)
Tion3sUartIO = tion.tion_ns.class_("Tion3sUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(Tion3sUartVPort, Tion3sUartIO, "60s")


async def to_code(config):
    await vport.setup_vport_uart(config)
    # enable ota subscription
    cg.add_define("USE_OTA_STATE_CALLBACK")

    if CORE.is_esp8266:
        # FIXME check twice
        cg.add_define("USE_TION_HALF_DUPLEX")
