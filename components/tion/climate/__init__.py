import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ICON, CONF_ID

from .. import ICON_AIR_FILTER, new_pc_component, pc_schema, tion_ns

TionClimate = tion_ns.class_("TionClimate2", climate.Climate, cg.Component)


def climate_climate_schema(class_: cg.MockObj):
    return climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(class_),
            cv.Optional(CONF_ICON, default=ICON_AIR_FILTER): cv.icon,
        }
    )


CONFIG_SCHEMA = pc_schema(climate_climate_schema(TionClimate), None)


async def climate_new_climate(config: dict, *args):
    var = cg.new_Pvariable(config[CONF_ID], *args)
    await climate.register_climate(var, config)
    return var


async def to_code(config: dict):
    await new_pc_component(config, climate_new_climate, None)
