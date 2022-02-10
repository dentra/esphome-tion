from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch
from esphome.const import PLATFORM_ESP32
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
AUTO_LOAD = ["tion"]


Tion3s = tion.tion_ns.class_("Tion3s", PollingComponent, climate.Climate)
Tion3sBuzzerSwitch = tion.tion_ns.class_("Tion3sBuzzerSwitch", switch.Switch)

CONFIG_SCHEMA = tion.tion_schema(Tion3s, Tion3sBuzzerSwitch)


async def to_code(config):
    """Generate code"""
    var = await tion.setup_tion_core(config)  # pylint: disable=unused-variable
