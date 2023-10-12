import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import uart
from esphome.const import CONF_ID
from .. import vport, tion  # pylint: disable=relative-beyond-top-level

tion_3s_proxy_ns = cg.esphome_ns.namespace("tion_3s_proxy")
Tion3sProxy = tion_3s_proxy_ns.class_("Tion3sProxy", cg.Component)
Tion3sApiProxy = tion_3s_proxy_ns.class_("Tion3sApiProxy")

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tion3sProxy),
            cv.GenerateID(tion.CONF_TION_API_BASE_ID): cv.declare_id(Tion3sApiProxy),
            cv.GenerateID(tion.CONF_TION_API_ID): cv.declare_id(tion.TionVPortApi),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
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
