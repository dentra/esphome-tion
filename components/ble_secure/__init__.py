import esphome.codegen as cg
from esphome.components import esp32_ble_tracker
import esphome.config_validation as cv
from esphome.const import CONF_ID, PLATFORM_ESP32
from esphome.cpp_types import Component

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["esp32_ble_tracker"]

ble_secure_ns = cg.esphome_ns.namespace("ble_secure")
BLESecure = ble_secure_ns.class_("BLESecure", Component)

CONFIG_SCHEMA = {
    cv.GenerateID(): cv.declare_id(BLESecure),
}


async def to_code(config):
    """Code generation entry point"""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add_build_flag("-DBLE_SECURE_ENABLED")
