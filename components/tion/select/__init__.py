import esphome.codegen as cg
from esphome.components import select
from esphome.const import CONF_ICON, CONF_ID

from .. import new_pc_component, pc_schema, tion_ns

TionSelect = tion_ns.class_("TionSelect", select.Select, cg.Component)

PROPERTIES = {
    "air_intake": {
        CONF_ICON: "mdi:valve",
    },
    "presets": {
        CONF_ICON: "mdi:tune",
    },
    # aliases
    "gate_position": "air_intake",
}

CONFIG_SCHEMA = pc_schema(select.select_schema(TionSelect), PROPERTIES)


async def select_new_select(config, *args):
    var = cg.new_Pvariable(config[CONF_ID], *args)
    await select.register_select(var, config, options=[])
    return var


async def to_code(config: dict):
    # select.new_select do not pass additional args to ctor, so use own impl
    await new_pc_component(config, select_new_select, PROPERTIES)
