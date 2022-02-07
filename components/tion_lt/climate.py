import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_types import PollingComponent
from esphome.components import climate, switch
from .. import tion
from esphome.const import PLATFORM_ESP32


CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
AUTO_LOAD = ["tion"]

TionLt = tion.tion_ns.class_("TionLt", PollingComponent, climate.Climate)
TionLtLedSwitch = tion.tion_ns.class_("TionLtLedSwitch", switch.Switch)
TionLtBuzzerSwitch = tion.tion_ns.class_("TionLtBuzzerSwitch", switch.Switch)

CONFIG_SCHEMA = tion.tion_schema(TionLt, TionLtLedSwitch, TionLtBuzzerSwitch)


async def to_code(config):
    var = await tion.setup_breezer(config)
