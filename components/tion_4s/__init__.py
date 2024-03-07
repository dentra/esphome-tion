import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG
from esphome.cpp_generator import MockObjClass

from .. import vport  # pylint: disable=relative-beyond-top-level
from .. import tion, tion_lt  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "number"]

CONF_RECIRCULATION = "recirculation"

CONF_HEARTBEAT_INTERVAL = "heartbeat_interval"

Tion4sApi = tion.tion_ns.class_("Tion4sApi")
TionRecirculationSwitchT = tion.tion_ns.class_("TionRecirculationSwitch", switch.Switch)

Tion4sApiComponent = tion.tion_ns.class_(
    "Tion4sApiComponent", cg.Component, tion.TionApiComponent
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tion4sApiComponent),
            cv.GenerateID(tion.CONF_TION_API_BASE_ID): cv.declare_id(Tion4sApi),
            cv.GenerateID(tion.CONF_TION_API_ID): cv.declare_id(tion.TionVPortApi),
            cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.update_interval,
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)


def tion_4s_schema(tion_class: MockObjClass, tion_base_schema: cv.Schema):
    return tion_lt.tion_lt_base_schema(tion_class, Tion4sApi, tion_base_schema).extend(
        {
            cv.Optional(CONF_RECIRCULATION): switch.switch_schema(
                TionRecirculationSwitchT.template(tion_class),
                icon="mdi:air-conditioner",
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
        }
    )


async def setup_tion_4s(config, compoent_reg):
    """Code generation entry point"""
    var = await tion_lt.setup_tion_lt(config, compoent_reg)
    await tion.setup_switch(config, CONF_RECIRCULATION, var.set_recirculation, var)


async def to_code(config: dict):
    var = await tion.setup_tion_api(config, "4s")
    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
