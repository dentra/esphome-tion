import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    ble_client,
    climate,
    switch,
    sensor,
    text_sensor,
    binary_sensor,
)
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_ID,
    CONF_INVERTED,
    CONF_VERSION,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_CONFIG,
    PLATFORM_ESP32,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_NONE,
    UNIT_CELSIUS,
)


CODEOWNERS = ["@dentra"]
ESP_PLATFORMS = [PLATFORM_ESP32]
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["tion-api", "sensor", "switch", "text_sensor", "binary_sensor"]

ICON_AIR_FILTER = "mdi:air-filter"

CONF_BUZZER = "buzzer"
CONF_TEMP_IN = "temp_in"
CONF_TEMP_OUT = "temp_out"
CONF_FILTER_DAYS_LEFT = "filter_days_left"

UNIT_DAYS = "days"

tion_ns = cg.esphome_ns.namespace("tion")


def tion_schema(tion_class, buzzer_class):
    """Declare base tion schema"""
    return (
        climate.CLIMATE_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(tion_class),
                cv.Optional(CONF_ICON, default=ICON_AIR_FILTER): cv.icon,
                cv.Optional(CONF_BUZZER): switch.SWITCH_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(buzzer_class),
                        cv.Optional(CONF_ICON, default="mdi:volume-high"): cv.icon,
                        cv.Optional(
                            CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                        ): cv.entity_category,
                        cv.Optional(CONF_INVERTED): cv.invalid(
                            "Inverted mode is not supported"
                        ),
                    }
                ),
                cv.Optional(CONF_TEMP_IN): sensor.sensor_schema(
                    unit_of_measurement=UNIT_CELSIUS,
                    accuracy_decimals=0,
                    device_class=DEVICE_CLASS_TEMPERATURE,
                    state_class=STATE_CLASS_MEASUREMENT,
                    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                ),
                cv.Optional(CONF_TEMP_OUT): sensor.sensor_schema(
                    unit_of_measurement=UNIT_CELSIUS,
                    accuracy_decimals=0,
                    device_class=DEVICE_CLASS_TEMPERATURE,
                    state_class=STATE_CLASS_MEASUREMENT,
                    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                ),
                cv.Optional(CONF_VERSION): text_sensor.TEXT_SENSOR_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(text_sensor.TextSensor),
                        cv.Optional(CONF_ICON, default="mdi:git"): cv.icon,
                        cv.Optional(
                            CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_DIAGNOSTIC
                        ): cv.entity_category,
                    }
                ),
                cv.Optional(CONF_FILTER_DAYS_LEFT): sensor.sensor_schema(
                    unit_of_measurement=UNIT_DAYS,
                    accuracy_decimals=0,
                    icon=ICON_AIR_FILTER,
                    state_class=STATE_CLASS_NONE,
                    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                ),
            }
        )
        .extend(ble_client.BLE_CLIENT_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
    )


async def setup_binary_sensor(config, key, setter):
    """Setup binary sensor"""
    if key not in config:
        return None
    conf = config[key]
    var = cg.new_Pvariable(conf[CONF_ID])
    await binary_sensor.register_binary_sensor(var, conf)
    cg.add(setter(var))
    return var


async def setup_switch(config, key, setter, parent):
    """Setup switch"""
    if key not in config:
        return None
    conf = config[key]
    var = cg.new_Pvariable(conf[CONF_ID], parent)
    await switch.register_switch(var, conf)
    cg.add(setter(var))
    return var


async def setup_sensor(config, key, setter):
    """Setup sensor"""
    if key not in config:
        return None
    var = await sensor.new_sensor(config[key])
    cg.add(setter(var))
    return var


async def setup_tion_core(config):
    """Setup core component properties"""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    await climate.register_climate(var, config)

    await setup_switch(config, CONF_BUZZER, var.set_buzzer, var)
    await setup_sensor(config, CONF_TEMP_IN, var.set_temp_in)
    await setup_sensor(config, CONF_TEMP_OUT, var.set_temp_out)
    await setup_sensor(config, CONF_FILTER_DAYS_LEFT, var.set_filter_days_left)

    if CONF_VERSION in config:
        conf = config[CONF_VERSION]
        sens = cg.new_Pvariable(conf[CONF_ID])
        await text_sensor.register_text_sensor(sens, conf)
        cg.add(var.set_version(sens))

    cg.add_build_flag("-DTION_ESPHOME")
    # cg.add_library("tion-api", None, "https://github.com/dentra/tion-api")

    return var
