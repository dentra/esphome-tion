import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from .. import vport, tion  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport", "tion"]

CONF_HEARTBEAT_INTERVAL = "heartbeat_interval"


TionUARTVPort = tion.tion_ns.class_("TionUARTVPort", cg.PollingComponent, vport.VPort)
VPortTionUartProtocol = tion.tion_ns.class_("VPortTionUartProtocol")

CONFIG_SCHEMA = vport.vport_uart_schema(
    TionUARTVPort, VPortTionUartProtocol, "10s"
).extend(
    {
        cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.Range(min=cv.TimePeriod(seconds=1), min_included=False),
        ),
    }
)


async def to_code(config):
    var = await vport.setup_vport_uart(config)
    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
    cg.add_build_flag("-DTION_ENABLE_HEARTBEAT")
    # enable ota subscription
    cg.add_define("USE_OTA_STATE_CALLBACK")
