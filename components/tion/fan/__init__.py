import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID

from .. import new_pc_component, pc_schema, tion_ns

TionFan = tion_ns.class_("TionFan", fan.Fan, cg.Component)


def fan_fan_schema(class_: cg.MockObj):
    return fan.FAN_SCHEMA.extend({cv.GenerateID(): cv.declare_id(class_)})


CONFIG_SCHEMA = pc_schema(fan_fan_schema(TionFan), None)


async def fan_new_climate(config: dict, *args):
    var = cg.new_Pvariable(config[CONF_ID], *args)
    await fan.register_fan(var, config)
    return var


async def to_code(config: dict):
    await new_pc_component(config, fan_new_climate, None)
