import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch, number

from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INVERTED,
    CONF_MODE,
    CONF_UNIT_OF_MEASUREMENT,
    ENTITY_CATEGORY_CONFIG,
    PLATFORM_ESP32,
)
from .. import tion, tion_lt  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
AUTO_LOAD = ["tion", "number"]

Tion4s = tion.tion_ns.class_("Tion4s", PollingComponent, climate.Climate)

Tion4sLedSwitch = tion.tion_ns.class_("Tion4sLedSwitch", switch.Switch)
Tion4sBuzzerSwitch = tion.tion_ns.class_("Tion4sBuzzerSwitch", switch.Switch)
Tion4sRecirculationSwitch = tion.tion_ns.class_(
    "Tion4sRecirculationSwitch", switch.Switch
)
Tion4sBoostTimeNumber = tion.tion_ns.class_("Tion4sBoostTimeNumber", number.Number)

CONF_RECIRCULATION = "recirculation"
CONF_BOOST_TIME = "boost_time"

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
        cv.Optional(CONF_BOOST_TIME): number.NUMBER_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(Tion4sBoostTimeNumber),
                cv.Optional(CONF_ICON, default="mdi:clock-fast"): cv.icon,
                cv.Optional(CONF_UNIT_OF_MEASUREMENT, default="s"): cv.string_strict,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(
                    number.NUMBER_MODES, upper=True
                ),
                cv.Optional(
                    CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                ): cv.entity_category,
            }
        ),
    }
)


async def to_code(config):
    """Generate code"""
    var = await tion_lt.setup_tion_lt(config)
    await tion.setup_switch(config, CONF_RECIRCULATION, var.set_recirculation, var)
    if CONF_BOOST_TIME in config:
        numb = await number.new_number(
            config[CONF_BOOST_TIME], min_value=1, max_value=20, step=1
        )
        cg.add(numb.set_parent(var))
        cg.add(var.set_boost_time(numb))
