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

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion"]

TionApiLt = tion.tion_ns.class_("TionApiLt")
TionLedSwitchT = tion.tion_ns.class_("TionLedSwitch", switch.Switch)

CONF_LED = "led"
CONF_HEATER_POWER = "heater_power"
CONF_AIRFLOW_COUNTER = "airflow_counter"


def tion_lt_base_schema(
    tion_class: MockObjClass, tion_api_class: MockObjClass, tion_base_schema: cv.Schema
):
    """Declare base tion lt schema"""
    return tion.tion_schema(tion_class, tion_api_class, tion_base_schema).extend(
        {
            cv.Optional(CONF_LED): switch.SWITCH_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(TionLedSwitchT.template(tion_class)),
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
        }
    )


def tion_lt_schema(tion_class: MockObjClass, tion_base_schema: cv.Schema):
    return tion_lt_base_schema(tion_class, TionApiLt, tion_base_schema)


async def setup_tion_lt(config, component_reg):
    """Setup tion lt component properties"""
    var = await tion.setup_tion_core(config, component_reg)
    await tion.setup_switch(config, CONF_LED, var.set_led, var)
    await tion.setup_sensor(config, CONF_HEATER_POWER, var.set_heater_power)
    await tion.setup_sensor(config, CONF_AIRFLOW_COUNTER, var.set_airflow_counter)
    return var
