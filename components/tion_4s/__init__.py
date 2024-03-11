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


CONFIG_SCHEMA = tion.tion_schema_api(
    tion.tion_ns.class_("Tion4sApiComponent", cg.Component, tion.TionApiComponent),
    Tion4sApi,
)


async def to_code(config: dict):
    await tion.setup_tion_api(config, "4s")
