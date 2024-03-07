import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.cpp_types import PollingComponent

from ... import tion  # pylint: disable=relative-beyond-top-level
from .. import (  # pylint: disable=relative-beyond-top-level
    CONF_HEARTBEAT_INTERVAL,
    setup_tion_4s,
    tion_4s_schema,
)

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion_4s"]

Tion4sClimate = tion.tion_ns.class_("Tion4sClimate", PollingComponent, climate.Climate)

CONFIG_SCHEMA = tion_4s_schema(Tion4sClimate, climate.CLIMATE_SCHEMA).extend(
    {
        cv.Optional(CONF_HEARTBEAT_INTERVAL, default="5s"): cv.update_interval,
    }
)


async def to_code(config):
    """Code generation entry point"""
    var = await setup_tion_4s(config, climate.register_climate)
    cg.add(var.set_heartbeat_interval(config[CONF_HEARTBEAT_INTERVAL]))
