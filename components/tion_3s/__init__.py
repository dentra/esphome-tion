import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass
from esphome.components import select
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
)
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "select"]

CONF_AIR_INTAKE = "air_intake"
CONF_TURN_OFF_BEFORE_HEAT = "turn_off_before_heat"

Tion3sApi = tion.tion_ns.class_("Tion3sApi")
Tion3sAirIntakeSelectT = tion.tion_ns.class_("Tion3sAirIntakeSelect", select.Select)

OPTIONS_AIR_INTAKE = ["Indoor", "Mixed", "Outdoor"]


def tion_3s_schema(tion_class: MockObjClass, tion_base_schema: cv.Schema):
    return tion.tion_schema(tion_class, Tion3sApi, tion_base_schema).extend(
        {
            cv.Optional(CONF_AIR_INTAKE): select.select_schema(
                Tion3sAirIntakeSelectT.template(tion_class),
                icon="mdi:air-conditioner",
                entity_category=ENTITY_CATEGORY_CONFIG,
            ),
            cv.Optional(CONF_TURN_OFF_BEFORE_HEAT, default=True): cv.boolean,
        }
    )


async def setup_tion_3s(config, compoent_reg):
    """Code generation entry point"""
    var = await tion.setup_tion_core(config, compoent_reg)
    await tion.setup_select(
        config, CONF_AIR_INTAKE, var.set_air_intake, var, OPTIONS_AIR_INTAKE
    )
    if config[CONF_TURN_OFF_BEFORE_HEAT]:
        cg.add_define("USE_TION_ENABLE_OFF_BEFORE_HEAT")
