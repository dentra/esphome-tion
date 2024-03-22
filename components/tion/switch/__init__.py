import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number, sensor, switch
from esphome.const import (
    CONF_CO2,
    CONF_DEVICE_CLASS,
    CONF_DURATION,
    CONF_ENTITY_CATEGORY,
    CONF_HEATER,
    CONF_ICON,
    CONF_ID,
    CONF_LAMBDA,
    CONF_TEMPERATURE,
    CONF_TYPE,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_OPENING,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_CONFIG,
)

from .. import (
    CONF_MAX_FAN_SPEED,
    CONF_MIN_FAN_SPEED,
    CONF_SETPOINT,
    CONF_TION_COMPONENT_CLASS,
    check_type,
    get_pc_info,
    new_pc_component,
    pc_schema,
    setup_number,
    tion_ns,
)
from ..number import TionNumber

TionSwitch = tion_ns.class_("TionSwitch", switch.Switch, cg.Component)

CONF_AUTO_KP = "kp"
CONF_AUTO_TI = "ti"
CONF_AUTO_DB = "db"

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
    "auto": {
        CONF_TION_COMPONENT_CLASS: "TionAutoSwitch",
        CONF_DEVICE_CLASS: DEVICE_CLASS_RUNNING,
        CONF_ICON: "mdi:fan-auto",
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
                cv.Optional(CONF_CO2): cv.use_id(sensor.Sensor),
                cv.Optional(CONF_SETPOINT): cv.int_range(500, 1300),
                cv.Inclusive(CONF_MIN_FAN_SPEED, "auto_fan_speed"): cv.int_range(0, 5),
                cv.Inclusive(CONF_MAX_FAN_SPEED, "auto_fan_speed"): cv.int_range(1, 6),
                cv.Inclusive(CONF_AUTO_KP, "auto_data"): cv.positive_float,
                cv.Inclusive(CONF_AUTO_TI, "auto_data"): cv.positive_float,
                cv.Inclusive(CONF_AUTO_DB, "auto_data"): cv.int_range(-100, 100),
                cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            }
        ),
        PROPERTIES,
    ),
    check_type(CONF_DURATION, "boost"),
    check_type(CONF_HEATER, "boost"),
    check_type(CONF_TEMPERATURE, "boost"),
    # check_type(CONF_BOOST_TIME, "boost"),
    # check_type(CONF_BOOST_TIME_LEFT, "boost"),
    check_type(CONF_MIN_FAN_SPEED, "auto"),
    check_type(CONF_MAX_FAN_SPEED, "auto"),
    check_type(CONF_SETPOINT, "auto"),
    check_type(CONF_CO2, "auto", required=True),
    check_type(CONF_AUTO_KP, "auto"),
    check_type(CONF_AUTO_TI, "auto"),
    check_type(CONF_AUTO_DB, "auto"),
    check_type(CONF_LAMBDA, "auto"),
)


async def switch_new_switch(config, *args):
    (_, props) = get_pc_info(config, PROPERTIES)
    if CONF_TION_COMPONENT_CLASS in props:
        # hacky inject CONF_TION_COMPONENT_CLASS into CONF_ID
        config[CONF_ID].type = props[CONF_TION_COMPONENT_CLASS]
    return await switch.new_switch(config, *args)


async def _setup_auto(config: dict, var):
    if config[CONF_TYPE] != "auto":
        return

    sens = await cg.get_variable(config[CONF_CO2])
    cg.add(var.register_co2_sensor(sens))

    if CONF_SETPOINT in config:
        cg.add(var.set_setpoint(config[CONF_SETPOINT]))

    if CONF_MIN_FAN_SPEED in config:
        cg.add(var.set_min_fan_speed(config[CONF_MIN_FAN_SPEED]))

    if CONF_MAX_FAN_SPEED in config:
        cg.add(var.set_max_fan_speed(config[CONF_MAX_FAN_SPEED]))

    if CONF_AUTO_KP in config and CONF_AUTO_TI in config and CONF_AUTO_DB in config:
        cg.add(
            var.set_data(
                config[CONF_AUTO_KP], config[CONF_AUTO_TI], config[CONF_AUTO_DB]
            )
        )

    if lambda_func := config.get(CONF_LAMBDA, None):
        lambda_template = await cg.process_lambda(
            lambda_func, [(cg.uint16, "x")], return_type=cg.uint8, capture=""
        )
        cg.add(var.set_update_func(lambda_template))


async def to_code(config: dict):
    var = await new_pc_component(config, switch.new_switch, PROPERTIES)
    # await setup_number(config, CONF_BOOST_TIME, var.set_boost_time, 1, 60, 1)
    # await setup_sensor(config, CONF_BOOST_TIME_LEFT, var.set_boost_time_left)

    # boost settings
    if CONF_DURATION in config:
        cg.add(var.set_boost_time(config[CONF_DURATION]))
    if CONF_HEATER in config:
        cg.add(var.set_boost_heater_state(config[CONF_HEATER]))
    if CONF_TEMPERATURE in config:
        cg.add(var.set_boost_target_temperture(config[CONF_TEMPERATURE]))

    await _setup_auto(config, var)
