import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ICON, CONF_ID

from .. import new_pc_component, pc_schema, tion_ns

TionClimate = tion_ns.class_("TionClimate", climate.Climate, cg.Component)

ICON_AIR_FILTER = "mdi:air-filter"

CONF_ENABLE_HEAT_COOL = "enable_heat_cool"
CONF_ENABLE_FAN_AUTO = "enable_fan_auto"


def climate_climate_schema(class_: cg.MockObj):
    return climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(class_),
            cv.Optional(CONF_ICON, default=ICON_AIR_FILTER): cv.icon,
            cv.Optional(CONF_ENABLE_HEAT_COOL): cv.boolean,
            cv.Optional(CONF_ENABLE_FAN_AUTO): cv.boolean,
        }
    )


CONFIG_SCHEMA = pc_schema(climate_climate_schema(TionClimate), None)


async def climate_new_climate(config: dict, *args):
    var = cg.new_Pvariable(config[CONF_ID], *args)
    await climate.register_climate(var, config)
    return var


async def to_code(config: dict):
    var = await new_pc_component(config, climate_new_climate, None)
    if CONF_ENABLE_HEAT_COOL in config:
        cg.add(var.set_enable_heat_cool(config[CONF_ENABLE_HEAT_COOL]))
    if CONF_ENABLE_FAN_AUTO in config:
        cg.add(var.set_enable_fan_auto(config[CONF_ENABLE_FAN_AUTO]))
