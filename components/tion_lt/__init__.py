import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, switch
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INVERTED,
    CONF_TEMPERATURE,
    DEVICE_CLASS_OPENING,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CUBIC_METER,
    UNIT_WATT,
)
from esphome.cpp_generator import MockObjClass
from esphome.cpp_types import PollingComponent

from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion"]

TionLtApi = tion.tion_ns.class_("TionLtApi")
TionLedSwitchT = tion.tion_ns.class_("TionLedSwitch", switch.Switch)

CONF_LED = "led"
CONF_HEATER_POWER = "heater_power"
CONF_AIRFLOW_COUNTER = "airflow_counter"
CONF_GATE_STATE = "gate_state"


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
    return tion_lt_base_schema(tion_class, TionLtApi, tion_base_schema).extend(
        {
            cv.Optional(CONF_GATE_STATE): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_OPENING,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )


async def setup_tion_lt(config, component_reg):
    """Setup tion lt component properties"""
    var = await tion.setup_tion_core(config, component_reg)
    await tion.setup_switch(config, CONF_LED, var.set_led, var)
    await tion.setup_sensor(config, CONF_HEATER_POWER, var.set_heater_power)
    await tion.setup_sensor(config, CONF_AIRFLOW_COUNTER, var.set_airflow_counter)
    await tion.setup_binary_sensor(config, CONF_GATE_STATE, var.set_gate_state)
    return var


CONF_FAN_SPEED = "fan_speed"
CONF_BUTTON_PRESETS = "button_presets"

TionLtApiComponent = tion.tion_ns.class_(
    "TionLtApiComponent", cg.Component, tion.TionApiComponent
)

BUTTON_PRESETS_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TEMPERATURE): cv.All(
            cv.ensure_list(cv.int_range(min=1, max=25)), cv.Length(min=3, max=3)
        ),
        cv.Required(CONF_FAN_SPEED): cv.All(
            cv.ensure_list(cv.int_range(min=1, max=6)), cv.Length(min=3, max=3)
        ),
    }
)

CONFIG_SCHEMA = tion.tion_schema_api(TionLtApiComponent, TionLtApi).extend(
    {
        cv.Optional(CONF_BUTTON_PRESETS): BUTTON_PRESETS_SCHEMA,
    }
)


async def to_code(config: dict):
    var = await tion.setup_tion_api(config, "lt")
    if CONF_BUTTON_PRESETS in config:
        button_presets = config[CONF_BUTTON_PRESETS]

        cg.add(
            var.set_button_presets(
                cg.StructInitializer(
                    "",
                    ("tmp", button_presets[CONF_TEMPERATURE]),
                    ("fan", button_presets[CONF_FAN_SPEED]),
                )
            )
        )
