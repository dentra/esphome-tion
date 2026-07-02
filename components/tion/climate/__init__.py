import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate

from .. import cgp, new_pc, tion_ns

TionClimate = tion_ns.class_("TionClimate", climate.Climate, cg.Component)


CONF_ENABLE_HEAT_COOL = "enable_heat_cool"
CONF_ENABLE_FAN_AUTO = "enable_fan_auto"
CONF_ENABLE_FAN_OFF = "enable_fan_off"
CONF_ENABLE_AIR_INTAKE_PRESET = "enable_air_intake_preset"

PC = new_pc(None)

CONFIG_SCHEMA = PC.climate_schema(
    TionClimate,
    icon=cgp.ICON_AIR_FILTER,
    ext_schema={
        cv.Optional(CONF_ENABLE_HEAT_COOL): cv.boolean,
        cv.Optional(CONF_ENABLE_FAN_AUTO): cv.boolean,
        cv.Optional(CONF_ENABLE_FAN_OFF): cv.boolean,
        cv.Optional(CONF_ENABLE_AIR_INTAKE_PRESET): cv.boolean,
    },
)


async def to_code(config: dict):
    var = await PC.new_climate(config)
    cgp.setup_value(config, CONF_ENABLE_HEAT_COOL, var.set_enable_heat_cool)
    cgp.setup_value(config, CONF_ENABLE_FAN_AUTO, var.set_enable_fan_auto)
    cgp.setup_value(config, CONF_ENABLE_FAN_OFF, var.set_enable_fan_off)
    cgp.setup_value(config, CONF_ENABLE_AIR_INTAKE_PRESET, var.set_enable_air_intake_preset)
