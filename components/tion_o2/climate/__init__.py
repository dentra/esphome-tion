from esphome.components import climate
from esphome.cpp_types import PollingComponent

# pylint: disable-next=relative-beyond-top-level
from ... import tion

# pylint: disable-next=relative-beyond-top-level
from .. import setup_tion_o2, tion_o2_schema

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion_o2"]


TionO2Climate = tion.tion_ns.class_("TionO2Climate", PollingComponent, climate.Climate)

CONFIG_SCHEMA = tion_o2_schema(TionO2Climate, climate.CLIMATE_SCHEMA)


async def to_code(config):
    """Code generation entry point"""
    # pylint: disable-next=unused-variable
    var = await setup_tion_o2(config, climate.register_climate)
