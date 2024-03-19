import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

from .. import tion, vport  # pylint: disable=relative-beyond-top-level

tion_3s_proxy_ns = cg.esphome_ns.namespace("tion_3s_proxy")
Tion3sBleProxy = tion_3s_proxy_ns.class_("Tion3sBleProxy", cg.Component)
Tion3sApiProxy = tion_3s_proxy_ns.class_("Tion3sApiProxy")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tion3sBleProxy),
            cv.GenerateID(tion.CONF_TION_ID): cv.declare_id(tion.TionVPortApi),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    _, api = await tion.new_vport_api_wrapper(config, Tion3sApiProxy)
    urt = await cg.get_variable(config[uart.CONF_UART_ID])
    ble = cg.new_Pvariable(config[CONF_ID], api, urt)
    await cg.register_component(ble, config)
