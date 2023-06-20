import esphome.config_validation as cv
from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch

from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
)
from .. import tion, tion_lt  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "number"]

Tion4s = tion.tion_ns.class_("Tion4s", PollingComponent, climate.Climate)
TionApi4s = tion.tion_ns.class_("TionApi4s")
TionRecirculationSwitchT = tion.tion_ns.class_("TionRecirculationSwitch", switch.Switch)

CONF_RECIRCULATION = "recirculation"


CONFIG_SCHEMA = tion_lt.tion_lt_schema(Tion4s, TionApi4s).extend(
    {
        cv.Optional(CONF_RECIRCULATION): switch.switch_schema(
            TionRecirculationSwitchT.template(Tion4s),
            icon="mdi:air-conditioner",
            entity_category=ENTITY_CATEGORY_CONFIG,
            block_inverted=True,
        ),
    }
)


async def to_code(config):
    """Code generation entry point"""
    var = await tion_lt.setup_tion_lt(config)
    await tion.setup_switch(config, CONF_RECIRCULATION, var.set_recirculation, var)
