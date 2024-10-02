import esphome.codegen as cg
from esphome.const import PLATFORM_ESP32

from .. import tion, vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["vport", "tion"]


TionLtBleVPort = tion.tion_ns.class_("TionLtBleVPort", cg.Component, vport.VPort)
TionLtBleIO = tion.tion_ns.class_("TionLtBleIO")

CONFIG_SCHEMA = vport.vport_ble_schema(TionLtBleVPort, TionLtBleIO)


async def to_code(config):
    await vport.setup_vport_ble(config)
