import esphome.codegen as cg
from esphome.components import select
from esphome.const import CONF_ICON

from .. import new_pc, cgp, tion_ns

TionSelect = tion_ns.class_("TionSelect", select.Select, cg.Component)

PC = new_pc(
    {
        "air_intake": {
            CONF_ICON: cgp.ICON_VALVE,
        },
        "presets": {
            CONF_ICON: cgp.ICON_TUNE,
        },
        # aliases
        "gate_position": "air_intake",
    }
)

CONFIG_SCHEMA = PC.select_schema(TionSelect)


async def to_code(config: dict):
    await PC.new_select(config)
