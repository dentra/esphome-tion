from esphome.cpp_types import PollingComponent
from esphome.components import climate
from .. import tion_lt_schema, setup_tion_lt
from ... import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion_lt"]

TionLtClimate = tion.tion_ns.class_("TionLtClimate", PollingComponent, climate.Climate)

CONFIG_SCHEMA = tion_lt_schema(TionLtClimate, climate.CLIMATE_SCHEMA)


async def to_code(config):
    """Code generation entry point"""
    # pylint: disable=unused-variable
    var = await setup_tion_lt(config, climate.register_climate)
