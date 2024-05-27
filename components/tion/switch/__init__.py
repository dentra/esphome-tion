import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, switch
from esphome.const import (
    CONF_CO2,
    CONF_DURATION,
    CONF_ENTITY_CATEGORY,
    CONF_HEATER,
    CONF_ICON,
    CONF_LAMBDA,
    CONF_TEMPERATURE,
    CONF_TYPE,
    ENTITY_CATEGORY_CONFIG,
)

from .. import (
    CONF_COMPONENT_CLASS,
    CONF_MAX_FAN_SPEED,
    CONF_MIN_FAN_SPEED,
    CONF_SETPOINT,
    new_pc,
    cgp,
    tion_ns,
)

TionSwitch = tion_ns.class_("TionSwitch", switch.Switch, cg.Component)

CONF_AUTO_KP = "kp"
CONF_AUTO_TI = "ti"
CONF_AUTO_DB = "db"

PC = new_pc(
    {
        "power": {
            CONF_ICON: cgp.ICON_POWER,
        },
        "heater": {
            CONF_ICON: cgp.ICON_RADIATOR,
        },
        "sound": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_VOLUME_HIGH,
        },
        "led": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_LED_OUTLINE,
        },
        "recirculation": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_VALVE,
        },
        "boost": {
            CONF_COMPONENT_CLASS: "TionBoostSwitch",
            CONF_ICON: cgp.ICON_ROCKET_LAUNCH,
        },
        "auto": {
            CONF_COMPONENT_CLASS: "TionAutoSwitch",
            CONF_ICON: cgp.ICON_FAN_AUTO,
        },
        # aliases
        "buzzer": "sound",
        "heat": "heater",
        "light": "led",
    }
)

CONFIG_SCHEMA = PC.switch_schema(
    TionSwitch,
    {
        cv.Optional(CONF_DURATION): cv.All(
            cv.positive_time_period_minutes,
            cv.Range(min=cv.TimePeriod(minutes=1), max=cv.TimePeriod(minutes=60)),
        ),
        cv.Optional(CONF_HEATER): cv.boolean,
        cv.Optional(CONF_TEMPERATURE): cv.int_range(-30, 30),
        cv.Optional(CONF_CO2): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SETPOINT): cv.int_range(500, 1300),
        cv.Inclusive(CONF_MIN_FAN_SPEED, "auto_fan_speed"): cv.int_range(0, 5),
        cv.Inclusive(CONF_MAX_FAN_SPEED, "auto_fan_speed"): cv.int_range(1, 6),
        cv.Inclusive(CONF_AUTO_KP, "auto_data"): cv.positive_float,
        cv.Inclusive(CONF_AUTO_TI, "auto_data"): cv.positive_float,
        cv.Inclusive(CONF_AUTO_DB, "auto_data"): cv.int_range(-100, 100),
        cv.Optional(CONF_LAMBDA): cv.returning_lambda,
    },
    [
        cgp.validate_type(CONF_DURATION, "boost"),
        cgp.validate_type(CONF_HEATER, "boost"),
        cgp.validate_type(CONF_TEMPERATURE, "boost"),
        cgp.validate_type(CONF_MIN_FAN_SPEED, "auto"),
        cgp.validate_type(CONF_MAX_FAN_SPEED, "auto"),
        cgp.validate_type(CONF_SETPOINT, "auto"),
        cgp.validate_type(CONF_CO2, "auto", required=True),
        cgp.validate_type(CONF_AUTO_KP, "auto"),
        cgp.validate_type(CONF_AUTO_TI, "auto"),
        cgp.validate_type(CONF_AUTO_DB, "auto"),
        cgp.validate_type(CONF_LAMBDA, "auto"),
    ],
)


async def _setup_auto(config: dict, var):
    if config[CONF_TYPE] != "auto":
        return

    await cgp.setup_variable(config, CONF_CO2, var.register_co2_sensor)

    cgp.setup_value(config, CONF_SETPOINT, var.set_setpoint)
    cgp.setup_value(config, CONF_MIN_FAN_SPEED, var.set_min_fan_speed)
    cgp.setup_value(config, CONF_MAX_FAN_SPEED, var.set_max_fan_speed)

    cgp.setup_values(config, [CONF_AUTO_KP, CONF_AUTO_TI, CONF_AUTO_DB], var.set_data)

    await cgp.setup_lambda(
        config,
        CONF_LAMBDA,
        var.set_update_func,
        parameters=[(cg.uint16, "x")],
        return_type=cg.uint8,
    )


async def to_code(config: dict):
    var = await PC.new_switch(config)

    # boost settings
    cgp.setup_value(config, CONF_DURATION, var.set_boost_time)
    cgp.setup_value(config, CONF_HEATER, var.set_boost_heater_state)
    cgp.setup_value(config, CONF_TEMPERATURE, var.set_boost_target_temperture)

    await _setup_auto(config, var)
