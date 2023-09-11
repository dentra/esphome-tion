import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass
from esphome.components import switch

from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
)
from .. import tion, tion_lt  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "number"]

CONF_RECIRCULATION = "recirculation"

TionApi4s = tion.tion_ns.class_("TionApi4s")
TionRecirculationSwitchT = tion.tion_ns.class_("TionRecirculationSwitch", switch.Switch)


def tion_4s_schema(tion_class: MockObjClass, tion_base_schema: cv.Schema):
    return tion_lt.tion_lt_base_schema(tion_class, TionApi4s, tion_base_schema).extend(
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
