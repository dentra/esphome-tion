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
            cv.GenerateID(tion.CONF_TION_API_BASE_ID): cv.declare_id(TionO2ApiProxy),
            cv.GenerateID(tion.CONF_TION_API_ID): cv.declare_id(tion.TionVPortApi),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    logging.warning("%s is not supported at this moment", config[CONF_PLATFORM])
    return

    prt = await vport.vport_get_var(config)
    api = cg.new_Pvariable(
        config[tion.CONF_TION_API_ID],
        cg.TemplateArguments(
            vport.vport_find(config).type.class_("frame_spec_type"),
            config[tion.CONF_TION_API_BASE_ID].type,
        ),
        prt,
    )
    urt = await cg.get_variable(config[uart.CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], api, urt)
    await cg.register_component(var, config)
