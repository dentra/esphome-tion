from esphome.cpp_types import PollingComponent
import esphome.config_validation as cv
from esphome.components import climate, switch, select
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    ENTITY_CATEGORY_CONFIG,
)
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "select"]

CONF_AIR_INTAKE = "air_intake"

Tion3s = tion.tion_ns.class_("Tion3s", PollingComponent, climate.Climate)
Tion3sBuzzerSwitch = tion.tion_ns.class_("Tion3sBuzzerSwitch", switch.Switch)
Tion3sAirIntakeSelect = tion.tion_ns.class_("Tion3sAirIntakeSelect", select.Select)

OPTIONS_AIR_INTAKE = ["Indoor", "Mixed", "Outdoor"]

CONFIG_SCHEMA = tion.tion_schema(Tion3s, Tion3sBuzzerSwitch).extend(
    {
        cv.Optional(CONF_AIR_INTAKE): select.SELECT_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(Tion3sAirIntakeSelect),
                cv.Optional(CONF_ICON, default="mdi:air-conditioner"): cv.icon,
                cv.Optional(
                    CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                ): cv.entity_category,
            }
        ),
    }
)


async def to_code(config):
    """Code generation entry point"""
    var = await tion.setup_tion_core(config)
    await tion.setup_select(
        config, CONF_AIR_INTAKE, var.set_air_intake, var, OPTIONS_AIR_INTAKE
    )
