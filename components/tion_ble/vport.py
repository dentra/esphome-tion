from esphome.const import PLATFORM_ESP32
from .. import tion, tion_ble  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["vport", "tion"]


VPortTionBleLtProtocol = tion.tion_ns.class_("VPortTionBleLtProtocol")

CONFIG_SCHEMA = tion_ble.tion_ble_schema(tion_ble.TionBLEVPort, VPortTionBleLtProtocol)


async def to_code(config):
    await tion_ble.setup_tion_ble(config)
