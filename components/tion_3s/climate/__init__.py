from esphome.cpp_types import PollingComponent
from esphome.components import climate

from ... import tion  # pylint: disable=relative-beyond-top-level
from .. import (
    tion_3s_schema,
    setup_tion_3s,
)  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion_3s"]


Tion3sClimate = tion.tion_ns.class_("Tion3sClimate", PollingComponent, climate.Climate)

CONFIG_SCHEMA = tion_3s_schema(Tion3sClimate, climate.CLIMATE_SCHEMA)


async def to_code(config):
    """Code generation entry point"""
    # pylint: disable=unused-variable
    var = await setup_tion_3s(config, climate.register_climate)
