import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    CONF_DEVICE_CLASS,
    CONF_DURATION,
    CONF_ENTITY_CATEGORY,
    CONF_HEATER,
    CONF_ICON,
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_TYPE,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_OPENING,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_CONFIG,
)

from .. import (
    CONF_TION_COMPONENT_CLASS,
    check_type,
    get_pc_info,
    new_pc_component,
    pc_schema,
    tion_ns,
)

TionSwitch = tion_ns.class_("TionSwitch", switch.Switch, cg.Component)

PROPERTIES = {
    "power": {
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
    "recirculation": {
        CONF_DEVICE_CLASS: DEVICE_CLASS_OPENING,
        # CONF_ICON: "mdi:valve", # mdi:air-conditioner
    },
    "boost": {
        CONF_TION_COMPONENT_CLASS: "TionBoostSwitch",
        CONF_DEVICE_CLASS: DEVICE_CLASS_RUNNING,
        CONF_ICON: "mdi:rocket-launch",
    },
    # aliases
    "buzzer": "sound",
    "heat": "heater",
    "light": "led",
}

CONFIG_SCHEMA = cv.All(
    pc_schema(
        switch.switch_schema(TionSwitch, block_inverted=True).extend(
            {
                cv.Optional(CONF_DURATION): cv.All(
                    cv.positive_time_period_minutes,
                    cv.Range(
                        min=cv.TimePeriod(minutes=1), max=cv.TimePeriod(minutes=60)
                    ),
                ),
                cv.Optional(CONF_HEATER): cv.boolean,
                cv.Optional(CONF_TEMPERATURE): cv.int_range(-30, 30),
                #         cv.Optional(CONF_BOOST_TIME): number.number_schema(
                #             TionNumber.template("BoostTime"),
                #             unit_of_measurement=UNIT_MINUTE,
                #             icon="mdi:clock-fast",
                #             entity_category=ENTITY_CATEGORY_CONFIG,
                #         ).extend(NUMBER_SCHEMA_EXT),
                #         cv.Optional(CONF_BOOST_TIME_LEFT): sensor.sensor_schema(
                #             unit_of_measurement=UNIT_SECOND,
                #             icon="mdi:clock-end",
                #             accuracy_decimals=1,
                #             device_class=DEVICE_CLASS_DURATION,
                #             state_class=STATE_CLASS_MEASUREMENT,
                #             entity_category=ENTITY_CATEGORY_NONE,
                #         ),
            }
        ),
        PROPERTIES,
    ),
    check_type(CONF_DURATION, "boost"),
    check_type(CONF_HEATER, "boost"),
    check_type(CONF_TEMPERATURE, "boost"),
    # check_type(CONF_BOOST_TIME, "boost"),
    # check_type(CONF_BOOST_TIME_LEFT, "boost"),
)


async def switch_new_switch(config, *args):
    (_, props) = get_pc_info(config, PROPERTIES)
    if CONF_TION_COMPONENT_CLASS in props:
        # hacky inject CONF_TION_COMPONENT_CLASS into CONF_ID
        config[CONF_ID].type = props[CONF_TION_COMPONENT_CLASS]
    return await switch.new_switch(config, *args)


async def to_code(config: dict):
    var = await new_pc_component(config, switch.new_switch, PROPERTIES)
    # await setup_number(config, CONF_BOOST_TIME, var.set_boost_time, 1, 60, 1)
    # await setup_sensor(config, CONF_BOOST_TIME_LEFT, var.set_boost_time_left)
    if CONF_DURATION in config:
        cg.add(var.set_boost_time(config[CONF_DURATION]))
    if CONF_HEATER in config:
        cg.add(var.set_boost_heater_state(config[CONF_HEATER]))
    if CONF_TEMPERATURE in config:
        cg.add(var.set_boost_target_temperture(config[CONF_TEMPERATURE]))
