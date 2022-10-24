import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import PLATFORM_ESP32
from .. import vport, tion, tion_ble  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["tion_ble", "vport", "tion"]

CONF_EXPERIMENTAL_ALWAYS_PAIR = "experimental_always_pair"

Tion3sBLEVPort = tion.tion_ns.class_("Tion3sBLEVPort", cg.PollingComponent, vport.VPort)
VPortTionBle3sProtocol = tion.tion_ns.class_("VPortTionBle3sProtocol")

CONFIG_SCHEMA = tion_ble.tion_ble_schema(Tion3sBLEVPort, VPortTionBle3sProtocol).extend(
    {
        cv.Optional(CONF_EXPERIMENTAL_ALWAYS_PAIR, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await tion_ble.setup_tion_ble(config)
    cg.add(var.set_experimental_always_pair(config[CONF_EXPERIMENTAL_ALWAYS_PAIR]))
