import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from .. import vport, tion  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport_uart", "tion"]

CONF_HEARTBEAT_INTERVAL = "heartbeat_interval"
CONF_PROTOCOL_ID = "protocol_id"

TionUARTVPort = tion.tion_ns.class_("TionUARTVPort", cg.PollingComponent, vport.VPort)
VPortTionUartProtocol = tion.tion_ns.class_("VPortTionUartProtocol")

CONFIG_SCHEMA = (
    vport.vport_schema(TionUARTVPort, "10s")
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(
        {
            cv.GenerateID(CONF_PROTOCOL_ID): cv.declare_id(VPortTionUartProtocol),
            cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(seconds=1), min_included=False),
            ),
        }
    )
)


async def to_code(config):
    uart_parent = await cg.get_variable(config[uart.CONF_UART_ID])
    prot = cg.new_Pvariable(config[CONF_PROTOCOL_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_parent, prot)
    await cg.register_component(var, config)
    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
    cg.add_build_flag("-DTION_ENABLE_HEARTBEAT")
