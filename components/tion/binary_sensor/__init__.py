import esphome.codegen as cg
from esphome.components import binary_sensor
from esphome.const import (
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    DEVICE_CLASS_DAMPER,
    DEVICE_CLASS_OPENING,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from .. import new_pc_component, pc_schema, tion_ns

TionBinarySensor = tion_ns.class_(
    "TionBinarySensor", binary_sensor.BinarySensor, cg.Component
)

PROPERTIES = {
    "power": {
        CONF_ICON: "mdi:power",
    },
    "heater": {
        CONF_ICON: "mdi:radiator",
    },
    "sound": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_ICON: "mdi:volume-high",
    },
    "led": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_ICON: "mdi:led-outline",
    },
    "filter": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        # CONF_ICON: "mdi:air-filter",
        CONF_DEVICE_CLASS: DEVICE_CLASS_PROBLEM,
    },
    "gate_position": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_OPENING,
        # CONF_DEVICE_CLASS: DEVICE_CLASS_DAMPER,
    },
    "heating": {
        # CONF_DEVICE_CLASS: DEVICE_CLASS_HEAT,
        CONF_ICON: "mdi:radiator",
    },
    "error": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_DEVICE_CLASS: DEVICE_CLASS_PROBLEM,
    },
    "gate_error": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_DEVICE_CLASS: DEVICE_CLASS_PROBLEM,
    },
    "boost": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_RUNNING,
        CONF_ICON: "mdi:rocket-launch",
    },
    "state": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_DEVICE_CLASS: DEVICE_CLASS_PROBLEM,
    },
    # "auto": {
    #     CONF_DEVICE_CLASS: DEVICE_CLASS_RUNNING,
    #     CONF_ICON: "mdi:fan-auto",
    # },
    # aliases
    "buzzer": "sound",
    "heat": "heater",
    "light": "led",
    "gate": "gate_position",
}

CONFIG_SCHEMA = pc_schema(
    binary_sensor.binary_sensor_schema(TionBinarySensor), PROPERTIES
)


async def to_code(config: dict):
    await new_pc_component(config, binary_sensor.new_binary_sensor, PROPERTIES)
