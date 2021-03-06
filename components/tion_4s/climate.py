import esphome.config_validation as cv
from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch

from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INVERTED,
    ENTITY_CATEGORY_CONFIG,
)
from .. import tion, tion_lt  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "number"]

Tion4s = tion.tion_ns.class_("Tion4s", PollingComponent, climate.Climate)

Tion4sLedSwitch = tion.tion_ns.class_("Tion4sLedSwitch", switch.Switch)
Tion4sBuzzerSwitch = tion.tion_ns.class_("Tion4sBuzzerSwitch", switch.Switch)
Tion4sRecirculationSwitch = tion.tion_ns.class_(
    "Tion4sRecirculationSwitch", switch.Switch
)

CONF_RECIRCULATION = "recirculation"


CONFIG_SCHEMA = tion_lt.tion_lt_schema(
    Tion4s, Tion4sLedSwitch, Tion4sBuzzerSwitch
).extend(
    {
        cv.Optional(CONF_RECIRCULATION): switch.SWITCH_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(Tion4sRecirculationSwitch),
                cv.Optional(CONF_ICON, default="mdi:air-conditioner"): cv.icon,
                cv.Optional(
                    CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                ): cv.entity_category,
                cv.Optional(CONF_INVERTED): cv.invalid(
                    "Inverted mode is not supported"
                ),
            }
        ),
    }
)


async def to_code(config):
    """Code generation entry point"""
    var = await tion_lt.setup_tion_lt(config)
    await tion.setup_switch(config, CONF_RECIRCULATION, var.set_recirculation, var)
