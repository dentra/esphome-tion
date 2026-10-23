import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble, switch
from esphome.components.esp32_ble import CONF_BLE_ID, ESP32BLE
from esphome.components.esp32_ble_server import BLEServer
from esphome.components.esp32_improv import CONF_BLE_SERVER_ID
from esphome.const import CONF_ADDRESS, CONF_ID, CONF_TYPE, ENTITY_CATEGORY_CONFIG

from ..tion import CONF_TION_ID, TionApiComponent

AUTO_LOAD = ["esp32_ble_server", "switch", "tion-api"]
CONFLICTS_WITH = ["tion_4s_ble", "tion_4s_uart"]

CONF_PAIR = "pair"

tion_rc_ns = cg.esphome_ns.namespace("tion_rc")
TionRC = tion_rc_ns.class_("TionRC", cg.Component)
TionRCPairSwitch = tion_rc_ns.class_("TionRCPairSwitch", switch.Switch, cg.Component)
TionRCControl = tion_rc_ns.class_("TionRCControl")

RC_TYPES = {
    "3s": tion_rc_ns.class_("Tion3sRC", TionRCControl),
    "4s": tion_rc_ns.class_("Tion4sRC", TionRCControl),
}

def FILTER_SOURCE_FILES() -> list[str]:
    # tion-api ships only the protocols of configured types, build only the matching RC
    from esphome.core import CORE

    rc_type = CORE.config.get("tion_rc", {}).get(CONF_TYPE)
    return [
        f"tion_rc_{typ}.{ext}"
        for typ in RC_TYPES
        if typ != rc_type
        for ext in ("h", "cpp")
    ]


# types that can work without a real breezer (built-in emulation)
RC_EMU_TYPES = ["4s"]


def _validate_emulation(config):
    if CONF_TION_ID not in config and config[CONF_TYPE] not in RC_EMU_TYPES:
        raise cv.Invalid(
            f"'{CONF_TION_ID}' is required for type '{config[CONF_TYPE]}', "
            f"emulation without a breezer is supported only for: {', '.join(RC_EMU_TYPES)}"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TionRC),
            cv.GenerateID(CONF_BLE_SERVER_ID): cv.use_id(BLEServer),
            cv.GenerateID(CONF_BLE_ID): cv.use_id(ESP32BLE),
            # without tion_id the breezer is emulated (4s only)
            cv.Optional(CONF_TION_ID): cv.use_id(TionApiComponent),
            cv.Optional(CONF_TYPE, default="4s"): cv.one_of(*RC_TYPES, lower=True),
            # advertise with this address instead of the one derived from ESP MAC,
            # e.g. to look like a known breezer
            cv.Optional(CONF_ADDRESS): cv.mac_address,
            cv.Required(CONF_PAIR): switch.switch_schema(
                TionRCPairSwitch,
                icon="mdi:bluetooth-connect",
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_emulation,
)


async def to_code(config):
    if CONF_TION_ID in config:
        tion = await cg.get_variable(config[CONF_TION_ID])
        ctl = RC_TYPES[config[CONF_TYPE]].new(tion.api())
    else:
        tion = cg.nullptr
        ctl = RC_TYPES[config[CONF_TYPE]].new(cg.nullptr)
        cg.add_define("USE_TION_RC_EMU")
        # tion component is absent, tion-api still needs to know it is built for ESPHome
        cg.add_build_flag("-DTION_ESPHOME")
    var = cg.new_Pvariable(config[CONF_ID], tion, ctl)

    await cg.register_component(var, config)
    # the server must exist before our setup() registers connect callbacks
    await cg.get_variable(config[CONF_BLE_SERVER_ID])

    ble = await cg.get_variable(config[CONF_BLE_ID])
    esp32_ble.register_gatts_event_handler(ble, var)
    esp32_ble.register_gap_event_handler(ble, var)

    if CONF_ADDRESS in config:
        cg.add(var.set_address(config[CONF_ADDRESS].as_hex))

    pair_sw = await switch.new_switch(config[CONF_PAIR])
    cg.add(pair_sw.set_parent(var))
    await cg.register_component(pair_sw, config[CONF_PAIR])
    cg.add(var.set_pair_mode(pair_sw))
