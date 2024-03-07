import esphome.codegen as cg
from esphome.components import text_sensor
from esphome.const import (
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from .. import new_pc_component, pc_schema, tion_ns

TionTextSensor = tion_ns.class_("TionTextSensor", text_sensor.TextSensor, cg.Component)

PROPERTIES = {
    "errors": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_ICON: "mdi:fan-alert",
        # CONF_DEVICE_CLASS: "enum",
    },
    "firmware_version": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_ICON: "mdi:git",
    },
    "hardware_version": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_ICON: "mdi:chip",
    },
    # aliases
    "hardware": "hardware_version",
    "firmware": "firmware_version",
    "version": "firmware_version",
}


CONFIG_SCHEMA = pc_schema(text_sensor.text_sensor_schema(TionTextSensor), PROPERTIES)


async def to_code(config: dict):
    await new_pc_component(config, text_sensor.new_text_sensor, PROPERTIES)
