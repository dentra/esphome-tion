import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PLATFORM

from .. import tion, vport  # pylint: disable=relative-beyond-top-level

tion_o2_proxy_ns = cg.esphome_ns.namespace("tion_o2_proxy")
TionO2Proxy = tion_o2_proxy_ns.class_("TionO2Proxy", cg.Component)
TionO2ApiProxy = tion_o2_proxy_ns.class_("TionO2ApiProxy")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TionO2Proxy),
            cv.GenerateID(tion.CONF_TION_ID): cv.declare_id(tion.TionVPortApi),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    logging.error("tion_o2_proxy is not supported at this moment")
    # _, api = await tion.new_vport_api_wrapper(config, TionO2ApiProxy)
    # urt = await cg.get_variable(config[uart.CONF_UART_ID])
    # var = cg.new_Pvariable(config[CONF_ID], api, urt)
    # await cg.register_component(var, config)
