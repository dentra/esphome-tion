from esphome.cpp_types import PollingComponent
import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass
from esphome.components import switch, sensor, binary_sensor
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_NONE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    CONF_ICON,
    CONF_INVERTED,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CUBIC_METER,
    UNIT_WATT,
)
from .. import tion  # pylint: disable=relative-beyond-top-level


CONF_LED = "led"
CONF_HEATER_POWER = "heater_power"
CONF_AIRFLOW_COUNTER = "airflow_counter"
CONF_FILTER_WARNOUT = "filter_warnout"


def tion_lt_schema(tion_class: MockObjClass, tion_api_class: MockObjClass):
    """Declare base tion lt schema"""
    return tion.tion_schema(tion_class, tion_api_class).extend(
        {
            cv.Optional(CONF_LED): switch.SWITCH_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(tion.TionSwitch),
                    cv.Optional(CONF_ICON, default="mdi:led-on"): cv.icon,
                    cv.Optional(
                        CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                    ): cv.entity_category,
                    cv.Optional(CONF_INVERTED): cv.invalid(
                        "Inverted mode is not supported"
                    ),
                }
            ),
            cv.Optional(CONF_HEATER_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_AIRFLOW_COUNTER): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER,
                accuracy_decimals=2,
                icon="mdi:weather-windy",
                state_class=STATE_CLASS_TOTAL_INCREASING,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_FILTER_WARNOUT): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_NONE,
            ),
        }
    )


async def setup_tion_lt(config):
    """Setup tion lt component properties"""
    var = await tion.setup_tion_core(config)
    await tion.setup_switch(config, CONF_LED, var.set_led, var)
    await tion.setup_sensor(config, CONF_HEATER_POWER, var.set_heater_power)
    await tion.setup_sensor(config, CONF_AIRFLOW_COUNTER, var.set_airflow_counter)
    await tion.setup_binary_sensor(config, CONF_FILTER_WARNOUT, var.set_filter_warnout)
    return var
