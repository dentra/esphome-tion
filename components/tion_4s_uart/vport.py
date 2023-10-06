import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import CORE
from .. import vport, tion  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport", "tion"]

CONF_HEARTBEAT_INTERVAL = "heartbeat_interval"

Tion4sUartVPort = tion.tion_ns.class_(
    "Tion4sUartVPort", cg.PollingComponent, vport.VPort
)
Tion4sUartIO = tion.tion_ns.class_("Tion4sUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(Tion4sUartVPort, Tion4sUartIO).extend(
    {
        cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.update_interval,
    }
)


async def to_code(config):
    var = await vport.setup_vport_uart(config)
    cg.add_build_flag("-DTION_ENABLE_HEARTBEAT")
    # enable ota subscription
    cg.add_define("USE_OTA_STATE_CALLBACK")

    if CORE.is_esp8266:
        # FIXME check twice
        cg.add_define("USE_TION_HALF_DUPLEX")

    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
