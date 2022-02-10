from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch
from esphome.const import PLATFORM_ESP32
from . import tion_lt_schema, setup_tion_lt
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
AUTO_LOAD = ["tion"]


TionLt = tion.tion_ns.class_("TionLt", PollingComponent, climate.Climate)
TionLtLedSwitch = tion.tion_ns.class_("TionLtLedSwitch", switch.Switch)
TionLtBuzzerSwitch = tion.tion_ns.class_("TionLtBuzzerSwitch", switch.Switch)


CONFIG_SCHEMA = tion_lt_schema(TionLt, TionLtLedSwitch, TionLtBuzzerSwitch)


async def to_code(config):
    """Generate code"""
    var = await setup_tion_lt(config)  # pylint: disable=unused-variable
