from esphome.cpp_types import PollingComponent
from esphome.components import climate
from .. import tion_lt_schema, setup_tion_lt
from ... import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion"]

TionLt = tion.tion_ns.class_("TionLt", PollingComponent, climate.Climate)
TionApiLt = tion.tion_ns.class_("TionApiLt")

CONFIG_SCHEMA = tion_lt_schema(TionLt, TionApiLt)


async def to_code(config):
    """Code generation entry point"""
    var = await setup_tion_lt(config)  # pylint: disable=unused-variable
