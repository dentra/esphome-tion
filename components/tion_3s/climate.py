from esphome.cpp_types import PollingComponent
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, switch, select
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
    PLATFORM_ESP32,
)
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
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
    if CONF_AIR_INTAKE in config:
        conf = config[CONF_AIR_INTAKE]
        sens = cg.new_Pvariable(conf[CONF_ID], var)
        await select.register_select(sens, conf, options=OPTIONS_AIR_INTAKE)
        cg.add(var.set_air_intake(sens))
