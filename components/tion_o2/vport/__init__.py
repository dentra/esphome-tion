import esphome.codegen as cg
import esphome.config_validation as cv

# pylint: disable-next=relative-beyond-top-level
from ... import tion, vport

AUTO_LOAD = ["vport", "tion"]

CONF_HEARTBEAT_INTERVAL = "heartbeat_interval"

TionO2UartVPort = tion.tion_ns.class_(
    "TionO2UartVPort", cg.PollingComponent, vport.VPort
)
TionO2UartIO = tion.tion_ns.class_("TionO2UartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(TionO2UartVPort, TionO2UartIO).extend(
    {
        cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.update_interval,
    }
)


async def to_code(config):
    var = await vport.setup_vport_uart(config)
    # enable ota subscription
    cg.add_define("USE_OTA_STATE_CALLBACK")
    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
