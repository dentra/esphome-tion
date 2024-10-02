import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import PLATFORM_ESP32

from .. import tion, vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["vport", "tion"]

CONF_EXPERIMENTAL_ALWAYS_PAIR = "experimental_always_pair"

Tion3sBleIO = tion.tion_ns.class_("Tion3sBleIO")
Tion3sBleVPort = tion.tion_ns.class_("Tion3sBleVPort", cg.Component, vport.VPort)

CONFIG_SCHEMA = vport.vport_ble_schema(Tion3sBleVPort, Tion3sBleIO).extend(
    {
        cv.Optional(CONF_EXPERIMENTAL_ALWAYS_PAIR, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await vport.setup_vport_ble(config)
    cg.add(var.set_experimental_always_pair(config[CONF_EXPERIMENTAL_ALWAYS_PAIR]))
    vio = await cg.get_variable(config[vport.CONF_VPORT_IO_ID])
    cg.add(vio.set_vport(var))
