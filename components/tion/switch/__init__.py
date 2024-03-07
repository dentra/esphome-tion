import voluptuous as vol

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number, sensor, switch
from esphome.const import (
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_ID,
    CONF_MODE,
    CONF_TYPE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_OPENING,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_NONE,
    STATE_CLASS_MEASUREMENT,
    UNIT_MINUTE,
    UNIT_SECOND,
)
from esphome.core import CORE, ID

from .. import (
    CONF_BOOST_TIME,
    CONF_BOOST_TIME_LEFT,
    CONF_TION_COMPONENT_CLASS,
    get_pc_info,
    new_pc_component,
    pc_schema,
    setup_number,
    setup_sensor,
    tion_ns,
)
from ..number import NUMBER_SCHEMA_EXT, TionNumber

TionSwitch = tion_ns.class_("TionSwitch", switch.Switch, cg.Component)
TionBoost = tion_ns.class_("TionBoost", switch.Switch, cg.Component)


PROPERTIES = {
    "power": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_POWER,
        CONF_ICON: "mdi:power",
    },
    "heater": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_HEAT,
        CONF_ICON: "mdi:radiator",
    },
    "sound": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
        CONF_ICON: "mdi:volume-high",
    },
    "led": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
        CONF_ICON: "mdi:led-outline",
    },
    "gate_position": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_OPENING,
        # CONF_ICON: "mdi:valve",
    },
    "boost": {
        # CONF_TION_COMPONENT_CLASS: TionBoost,
        CONF_DEVICE_CLASS: DEVICE_CLASS_RUNNING,
        CONF_ICON: "mdi:rocket-launch",
    },
    # aliases
    "buzzer": "sound",
    "heat": "heater",
    "light": "led",
    "recirculation": "gate_position",
    "gate": "gate_position",
}


def check_type(key, typ):
    def validator(config):
        if key in config and config[CONF_TYPE] != typ:
            raise cv.Invalid(f"{key} is not valid for the type {typ}")
        return config

    return validator


CONFIG_SCHEMA = cv.All(
    pc_schema(
        switch.switch_schema(TionSwitch, block_inverted=True).extend(
            {
                cv.Optional(CONF_BOOST_TIME): number.number_schema(
                    TionNumber.template("BoostTime"),
                    unit_of_measurement=UNIT_MINUTE,
                    icon="mdi:clock-fast",
                    entity_category=ENTITY_CATEGORY_CONFIG,
                ).extend(NUMBER_SCHEMA_EXT),
                cv.Optional(CONF_BOOST_TIME_LEFT): sensor.sensor_schema(
                    unit_of_measurement=UNIT_SECOND,
                    icon="mdi:clock-end",
                    accuracy_decimals=1,
                    device_class=DEVICE_CLASS_DURATION,
                    state_class=STATE_CLASS_MEASUREMENT,
                    entity_category=ENTITY_CATEGORY_NONE,
                ),
            }
        ),
        PROPERTIES,
    ),
    check_type(CONF_BOOST_TIME, "boost"),
    check_type(CONF_BOOST_TIME_LEFT, "boost"),
)


async def switch_new_switch(config, *args):
    (_, props) = get_pc_info(config, PROPERTIES)
    if CONF_TION_COMPONENT_CLASS in props:
        # hacky inject CONF_TION_COMPONENT_CLASS into CONF_ID
        config[CONF_ID].type = props[CONF_TION_COMPONENT_CLASS]
    return await switch.new_switch(config, *args)


async def to_code(config: dict):
    var = await new_pc_component(config, switch_new_switch, PROPERTIES)

    await setup_number(config, CONF_BOOST_TIME, var.set_boost_time, 1, 60, 1)
    await setup_sensor(config, CONF_BOOST_TIME_LEFT, var.set_boost_time_left)
