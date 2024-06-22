import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core
from esphome.components import switch
from esphome.components.esp32_ble import CONF_BLE_ID, ESP32BLE
from esphome.components.esp32_ble_server import BLEServer, BLEServiceComponent
from esphome.components.esp32_improv import CONF_BLE_SERVER_ID
from esphome.const import CONF_ID, CONF_TYPE, ENTITY_CATEGORY_CONFIG

from ..tion import CONF_TION_ID, TionApiComponent

AUTO_LOAD = ["esp32_ble_server"]
CONFLICTS_WITH = ["tion_4s_ble", "tion_4s_uart"]

CONF_PAIR = "pair"

tion_rc_ns = cg.esphome_ns.namespace("tion_rc")
TionRC = tion_rc_ns.class_("TionRC", cg.Component, BLEServiceComponent)
TionRCPairSwitch = tion_rc_ns.class_("TionRCPairSwitch", switch.Switch, cg.Component)
TionRCControl = tion_rc_ns.class_("TionRCControl")

RC_TYPES = {
    "3s": tion_rc_ns.class_("Tion3sRC", TionRCControl),
    "4s": tion_rc_ns.class_("Tion4sRC", TionRCControl),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TionRC),
        cv.GenerateID(CONF_BLE_SERVER_ID): cv.use_id(BLEServer),
        cv.GenerateID(CONF_BLE_ID): cv.use_id(ESP32BLE),
        cv.GenerateID(CONF_TION_ID): cv.use_id(TionApiComponent),
        cv.Optional(CONF_TYPE, default="4s"): cv.one_of(*RC_TYPES, lower=True),
        cv.Required(CONF_PAIR): switch.switch_schema(
            TionRCPairSwitch,
            icon="mdi:bluetooth-connect",
            entity_category=ENTITY_CATEGORY_CONFIG,
            block_inverted=True,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    api = await cg.get_variable(config[CONF_TION_ID])
    ctl = RC_TYPES[config[CONF_TYPE]].new(api.api())
    var = cg.new_Pvariable(config[CONF_ID], api, ctl)

    await cg.register_component(var, config)
    ble_server = await cg.get_variable(config[CONF_BLE_SERVER_ID])
    cg.add(ble_server.register_service_component(var))

    ble = await cg.get_variable(config[CONF_BLE_ID])
    cg.add(ble.register_gatts_event_handler(var))
    cg.add(ble.register_gap_event_handler(var))

    pair_sw = await switch.new_switch(config[CONF_PAIR])
    cg.add(pair_sw.set_parent(var))
    await cg.register_component(pair_sw, config[CONF_PAIR])
    cg.add(var.set_pair_mode(pair_sw))
