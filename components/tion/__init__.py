import logging
from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor as esphome_binary_sensor
from esphome.components import button, climate
from esphome.components import number as esphome_number
from esphome.components import select
from esphome.components import sensor as esphome_sensor
from esphome.components import switch, text_sensor
from esphome.const import (
    CONF_ACCURACY_DECIMALS,
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_FORCE_UPDATE,
    CONF_HEATER,
    CONF_ICON,
    CONF_ID,
    CONF_POWER,
    CONF_STATE_CLASS,
    CONF_TYPE,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_VERSION,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_NONE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_CUBIC_METER_PER_HOUR,
    UNIT_MINUTE,
    UNIT_SECOND,
)
from esphome.core import ID
from esphome.cpp_generator import MockObjClass

from .. import vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
# ESP_PLATFORMS = [PLATFORM_ESP32]
# DEPENDENCIES = ["ble_client"]
AUTO_LOAD = [
    "etl",
    "tion-api",
    "sensor",
    "switch",
    "text_sensor",
    "binary_sensor",
    "number",
    "select",
    "climate",
    "button",
]

_LOGGER = logging.getLogger(__name__)

ICON_AIR_FILTER = "mdi:air-filter"

CONF_TION_API_BASE_ID = "tion_api_base_id"
CONF_TION_API_ID = "tion_api_id"
CONF_TION_COMPONENT_CLASS = "tion_component_class"

CONF_BUZZER = "buzzer"
CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_FILTER_TIME_LEFT = "filter_time_left"
CONF_BOOST_TIME = "boost_time"
CONF_BOOST_TIME_LEFT = "boost_time_left"
CONF_PRESETS = "presets"
CONF_PRESET_MODE = "mode"
CONF_PRESET_FAN_SPEED = "fan_speed"
CONF_PRESET_TARGET_TEMPERATURE = "target_temperature"
CONF_PRESET_GATE_POSITION = "gate_position"
CONF_RESET_FILTER = "reset_filter"
CONF_RESET_FILTER_CONFIRM = "reset_filter_confirm"
CONF_STATE_TIMEOUT = "state_timeout"
CONF_STATE_WARNOUT = "state_warnout"
CONF_FILTER_WARNOUT = "filter_warnout"
CONF_PRODUCTIVITY = "productivity"
CONF_BATCH_TIMEOUT = "batch_timeout"
CONF_ERRORS = "errors"
CONF_WORK_TIME = "work_time"

UNIT_DAYS = "d"

tion_ns = cg.esphome_ns.namespace("tion")
TionBoostTimeNumber = tion_ns.class_("TionBoostTimeNumber", esphome_number.Number)
TionBuzzerSwitchT = tion_ns.class_("TionBuzzerSwitch", switch.Switch)
TionVPortApi = tion_ns.class_("TionVPortApi")
TionResetFilterButtonT = tion_ns.class_("TionResetFilterButton", button.Button)
TionResetFilterConfirmSwitchT = tion_ns.class_(
    "TionResetFilterConfirmSwitch", switch.Switch
)
TionGatePosition = tion_ns.namespace("TionGatePosition")

PRESET_MODES = {
    "off": climate.ClimateMode.CLIMATE_MODE_OFF,
    "heat": climate.ClimateMode.CLIMATE_MODE_HEAT,
    "fan_only": climate.ClimateMode.CLIMATE_MODE_FAN_ONLY,
    "heat_cool": climate.ClimateMode.CLIMATE_MODE_HEAT_COOL,
}

PRESET_GATE_POSITIONS = {
    "none": TionGatePosition.NONE,
    "outdoor": TionGatePosition.OUTDOOR,
    "indoor": TionGatePosition.INDOOR,
    "mixed": TionGatePosition.MIXED,
}

PRESETS = []

PRESET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_PRESET_MODE): cv.one_of(*PRESET_MODES, lower=True),
        cv.Optional(CONF_PRESET_FAN_SPEED): cv.int_range(min=1, max=6),
        cv.Optional(CONF_PRESET_TARGET_TEMPERATURE): cv.int_range(min=1, max=30),
        cv.Optional(CONF_PRESET_GATE_POSITION): cv.one_of(
            *PRESET_GATE_POSITIONS, lower=True
        ),
    }
)

PRESETS_SCHEMA = cv.Schema(
    {
        cv.Optional("home"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("away"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("boost"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("comfort"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("eco"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("sleep"): cv.Any(PRESET_SCHEMA, None),
        cv.Optional("activity"): cv.Any(PRESET_SCHEMA, None),
    }
)

CUSTOM_PRESET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_POWER, default=True): cv.boolean,
        cv.Optional(CONF_HEATER, default=True): cv.boolean,
        cv.Optional(CONF_PRESET_FAN_SPEED): cv.int_range(min=1, max=6),
        cv.Optional(CONF_PRESET_TARGET_TEMPERATURE, default=0): cv.int_range(
            min=-30, max=30
        ),
        cv.Optional(CONF_PRESET_GATE_POSITION, default="none"): cv.one_of(
            *PRESET_GATE_POSITIONS, lower=True
        ),
    }
)


def tion_schema_api(
    tion_class: MockObjClass,
    tion_api_class: MockObjClass,
    tion_base_schema: cv.Schema = cv.Schema({}),
    polling_interval: str = "60s",
):
    return (
        tion_base_schema.extend(
            {
                cv.GenerateID(): cv.declare_id(tion_class),
                cv.GenerateID(CONF_TION_API_BASE_ID): cv.declare_id(tion_api_class),
                cv.GenerateID(CONF_TION_API_ID): cv.declare_id(TionVPortApi),
                cv.Optional(CONF_STATE_TIMEOUT, default="3s"): cv.update_interval,
                cv.Optional(
                    CONF_BATCH_TIMEOUT, default="200ms"
                ): cv.positive_time_period_milliseconds,
                # cv.Optional(CONF_BOOST_TIME, default="10min"): cv.All(
                #     cv.positive_time_period_milliseconds,
                #     cv.Range(
                #         min=cv.TimePeriod(minutes=1),
                #         max=cv.TimePeriod(minutes=60),
                #     ),
                # ),
                cv.Optional(CONF_FORCE_UPDATE): cv.boolean,
                cv.Required(CONF_PRESETS): cv.Schema(
                    {cv.string_strict: CUSTOM_PRESET_SCHEMA}
                ),
            }
        )
        .extend(vport.VPORT_CLIENT_SCHEMA)
        .extend(cv.polling_component_schema(polling_interval))
    )


def tion_schema(
    tion_class: MockObjClass, tion_api_class: MockObjClass, tion_base_schema: cv.Schema
):
    """Declare base tion schema"""
    return tion_schema_api(tion_class, tion_api_class, tion_base_schema).extend(
        {
            cv.Optional(CONF_ICON, default=ICON_AIR_FILTER): cv.icon,
            cv.Optional(CONF_STATE_WARNOUT): esphome_binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_BUZZER): switch.switch_schema(
                TionBuzzerSwitchT.template(tion_class),
                icon="mdi:volume-high",
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            cv.Optional(CONF_OUTDOOR_TEMPERATURE): esphome_sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_NONE,
            ),
            cv.Optional(CONF_VERSION): text_sensor.text_sensor_schema(
                text_sensor.TextSensor,
                icon="mdi:git",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_FILTER_TIME_LEFT): esphome_sensor.sensor_schema(
                unit_of_measurement=UNIT_DAYS,
                accuracy_decimals=0,
                icon=ICON_AIR_FILTER,
                # device_class=DEVICE_CLASS_DURATION,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_NONE,
            ),
            cv.Optional(CONF_BOOST_TIME): esphome_number.number_schema(
                TionBoostTimeNumber,
                icon="mdi:clock-fast",
                unit_of_measurement=UNIT_MINUTE,
                entity_category=ENTITY_CATEGORY_CONFIG,
            ),
            cv.Optional(CONF_BOOST_TIME_LEFT): esphome_sensor.sensor_schema(
                unit_of_measurement=UNIT_SECOND,
                accuracy_decimals=1,
                icon="mdi:clock-end",
                device_class=DEVICE_CLASS_DURATION,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_NONE,
            ),
            cv.Optional(CONF_PRESETS): PRESETS_SCHEMA,
            cv.Optional(CONF_RESET_FILTER): button.button_schema(
                TionResetFilterButtonT.template(tion_class),
                icon="mdi:wrench-cog",
                entity_category=ENTITY_CATEGORY_CONFIG,
            ),
            cv.Optional(CONF_RESET_FILTER_CONFIRM): switch.switch_schema(
                TionResetFilterConfirmSwitchT.template(tion_class),
                icon="mdi:wrench-check",
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            cv.Optional(
                CONF_FILTER_WARNOUT
            ): esphome_binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_NONE,
            ),
            cv.Optional(CONF_PRODUCTIVITY): esphome_sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER_PER_HOUR,
                accuracy_decimals=0,
                icon="mdi:weather-windy",
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ERRORS): text_sensor.text_sensor_schema(
                icon="mdi:alert",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_WORK_TIME): esphome_sensor.sensor_schema(
                unit_of_measurement=UNIT_SECOND,
                accuracy_decimals=0,
                icon="mdi:power",
                device_class=DEVICE_CLASS_DURATION,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )


def tion_vport_ble_schema(vport_class: MockObjClass, io_class: MockObjClass):
    return vport.vport_ble_schema(vport_class, io_class)


async def setup_binary_sensor(config: dict, key: str, setter):
    """Setup binary sensor"""
    if key not in config:
        return None
    sens = await esphome_binary_sensor.new_binary_sensor(config[key])
    cg.add(setter(sens))
    cg.add_define(f"USE_TION_{key.upper()}")
    return sens


async def setup_switch(config: dict, key: str, setter, parent):
    """Setup switch"""
    if key not in config:
        return None
    swch = await switch.new_switch(config[key], parent)
    cg.add(setter(swch))
    cg.add_define(f"USE_TION_{key.upper()}")
    return swch


async def setup_sensor(config: dict, key: str, setter):
    """Setup sensor"""
    if key not in config:
        return None
    sens = await esphome_sensor.new_sensor(config[key])
    cg.add(setter(sens))
    cg.add_define(f"USE_TION_{key.upper()}")
    return sens


async def setup_text_sensor(config: dict, key: str, setter):
    """Setup text sensor"""
    if key not in config:
        return None
    sens = await text_sensor.new_text_sensor(config[key])
    cg.add(setter(sens))
    cg.add_define(f"USE_TION_{key.upper()}")
    return sens


async def setup_number(
    config: dict, key: str, setter, min_value: int, max_value: int, step: int
):
    """Setup number"""
    if key not in config:
        return None
    numb = await esphome_number.new_number(
        config[key], min_value=min_value, max_value=max_value, step=step
    )
    cg.add(setter(numb))
    cg.add_define(f"USE_TION_{key.upper()}")
    return numb


async def setup_select(config: dict, key: str, setter, parent, options):
    """Setup select"""
    if key not in config:
        return None
    conf = config[key]
    slct = cg.new_Pvariable(conf[CONF_ID], parent)
    await select.register_select(slct, conf, options=options)
    cg.add(setter(slct))
    cg.add_define(f"USE_TION_{key.upper()}")
    return slct


async def setup_presets(config, key, setter) -> bool:
    """Setup presets"""
    if key not in config:
        return False
    conf = config[key]

    for preset_name, preset in climate.CLIMATE_PRESETS.items():
        preset_name = preset_name.lower()
        if preset_name == "none":
            continue
        if preset_name not in conf:
            # use OFF to disable preset
            cg.add(setter(preset, climate.ClimateMode.CLIMATE_MODE_OFF))
            continue

        options = conf[preset_name] or {}
        mode = (
            PRESET_MODES[options[CONF_PRESET_MODE]]
            if CONF_PRESET_MODE in options
            else climate.ClimateMode.CLIMATE_MODE_AUTO  # use AUTO to do not touch default mode
        )
        fan_speed = options.get(CONF_PRESET_FAN_SPEED, 0)
        target_temperature = options.get(CONF_PRESET_TARGET_TEMPERATURE, 0)
        gate_position_str = options.get(CONF_PRESET_GATE_POSITION, "none")
        gate_position = PRESET_GATE_POSITIONS[gate_position_str]
        cg.add(setter(preset, mode, fan_speed, target_temperature, gate_position))

    cg.add_build_flag("-DTION_ENABLE_PRESETS")

    return True


async def setup_button(config: dict, key: str, setter, parent):
    """Setup button"""
    if key not in config:
        return None
    butn = await button.new_button(config[key], parent)
    cg.add(setter(butn))
    cg.add_define(f"USE_TION_{key.upper()}")
    return butn


async def setup_tion_api(config: dict, component_source: str = None):
    prt = await vport.vport_get_var(config)
    api = cg.new_Pvariable(
        config[CONF_TION_API_ID],
        cg.TemplateArguments(
            # cg.MockObj(prt_io_id.type, "::").frame_spec_type,
            vport.vport_find(config).type.class_("frame_spec_type"),
            config[CONF_TION_API_BASE_ID].type,
        ),
        prt,
    )
    cg.add(prt.set_api(api))

    var = cg.new_Pvariable(config[CONF_ID], api, prt.get_type())
    await cg.register_component(var, config)
    if component_source:
        cg.add(var.set_component_source(f"tion_{component_source}"))

    # cg.add_library("tion-api", None, "https://github.com/dentra/tion-api")
    cg.add_build_flag("-DTION_ESPHOME")

    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    cg.add(var.set_batch_timeout(config[CONF_BATCH_TIMEOUT]))
    if CONF_FORCE_UPDATE in config:
        cg.add(var.set_force_update(config[CONF_FORCE_UPDATE]))
    if CONF_BOOST_TIME in config:
        cg.add(var.set_boost_time(config[CONF_BOOST_TIME]))

    if CONF_PRESETS in config:
        presets = set()
        for preset in config[CONF_PRESETS]:
            preset_name = str(preset).strip()
            if preset_name.lower() == "none":
                _LOGGER.warning("Preset 'none' is forbidden")
                continue
            if preset_name.lower() in presets:
                _LOGGER.warning("Preset '%s' already added", preset)
                continue
            preset = config[CONF_PRESETS][preset_name]
            cg.add(
                var.add_preset(
                    preset_name,
                    cg.StructInitializer(
                        "",
                        ("target_temperature", preset[CONF_PRESET_TARGET_TEMPERATURE]),
                        ("heater_state", preset[CONF_HEATER]),
                        ("power_state", preset[CONF_POWER]),
                        ("fan_speed", preset[CONF_PRESET_FAN_SPEED]),
                        (
                            "gate_position",
                            PRESET_GATE_POSITIONS[preset[CONF_PRESET_GATE_POSITION]],
                        ),
                    ),
                )
            )

            presets.add(preset_name)

    return var


async def setup_tion_core(config, component_reg):
    """Setup core component properties"""

    var = await setup_tion_api(config)

    await component_reg(var, config)

    await setup_switch(config, CONF_BUZZER, var.set_buzzer, var)
    await setup_sensor(config, CONF_OUTDOOR_TEMPERATURE, var.set_outdoor_temperature)
    await setup_sensor(config, CONF_FILTER_TIME_LEFT, var.set_filter_time_left)
    await setup_text_sensor(config, CONF_VERSION, var.set_version)
    has_presets = await setup_presets(config, CONF_PRESETS, var.update_preset)
    if has_presets and "boost" in config[CONF_PRESETS]:
        await setup_number(config, CONF_BOOST_TIME, var.set_boost_time, 1, 60, 1)
        await setup_sensor(config, CONF_BOOST_TIME_LEFT, var.set_boost_time_left)
    await setup_button(config, CONF_RESET_FILTER, var.set_reset_filter, var)
    await setup_switch(
        config, CONF_RESET_FILTER_CONFIRM, var.set_reset_filter_confirm, var
    )
    await setup_binary_sensor(config, CONF_FILTER_WARNOUT, var.set_filter_warnout)
    await setup_sensor(config, CONF_PRODUCTIVITY, var.set_productivity)
    await setup_text_sensor(config, CONF_ERRORS, var.set_errors)
    await setup_sensor(config, CONF_WORK_TIME, var.set_work_time)
    await setup_binary_sensor(config, CONF_STATE_WARNOUT, var.set_state_warnout)

    cg.add_define("USE_TION_CLIMATE")

    return var


async def setup_tion_vport_ble(config):
    var = await vport.setup_vport_ble(config)
    return var


TionApiComponent = tion_ns.class_("TionApiComponent")


def pc_schema(schema: cv.Schema, pcfg: dict):
    schema = schema.extend(
        {cv.GenerateID(CONF_TION_API_ID): cv.use_id(TionApiComponent)}
    )
    if pcfg:
        schema = schema.extend(
            {cv.Required(CONF_TYPE): cv.one_of(*pcfg.keys(), lower=True)}
        )
    return schema.extend(cv.COMPONENT_SCHEMA)


def get_pc_info(
    config: dict, pcfg: dict[str, str | dict[str, str]]
) -> tuple[str, dict[str, Any]]:
    pc_typ = config[CONF_TYPE] if CONF_TYPE in config else ""

    props: dict[str, Any] = None
    if pc_typ and pcfg:
        props = pcfg[pc_typ]
        if isinstance(props, str):
            pc_typ = props
            props = pcfg[pc_typ]
    return pc_typ, props


async def new_pc_component(
    config: dict,
    ctor,
    pcfg: dict[str, str | dict[str, str]] | None,
    **kwargs,
):
    (pc_typ, props) = get_pc_info(config, pcfg)

    def get_ctyp() -> str:
        import inspect

        stack = inspect.stack()
        frame = stack[2][0]
        ctyp = inspect.getmodule(frame).__name__.split(".")[-1:][0]
        ctyp = f"{ctyp}_" if ctyp == "switch" else ctyp

        return ctyp

    ctyp = get_ctyp()

    def get_pc_class() -> str:
        typ = pc_typ.replace("_", " ").title().replace(" ", "")
        return tion_ns.class_(f"property_controller::{ctyp}::{typ}")

    def set_prop(var: cg.MockObj, key: str):
        if not props or key in config:
            return
        if key in props:
            val = props[key]
            if key == CONF_ENTITY_CATEGORY:
                val = cv.ENTITY_CATEGORIES[val]
            elif key == CONF_STATE_CLASS:
                val = esphome_sensor.STATE_CLASSES[val]
            cg.add(getattr(var, f"set_{key}")(val))

    api: ID = config[CONF_TION_API_ID]
    parent = await cg.get_variable(api)
    if props and CONF_TION_COMPONENT_CLASS not in props:
        var = await ctor(config, cg.TemplateArguments(get_pc_class()), parent, **kwargs)
    else:
        var = await ctor(config, parent, **kwargs)

    set_prop(var, CONF_ENTITY_CATEGORY)
    set_prop(var, CONF_DEVICE_CLASS)
    set_prop(var, CONF_STATE_CLASS)
    set_prop(var, CONF_ICON)
    set_prop(var, CONF_UNIT_OF_MEASUREMENT)
    set_prop(var, CONF_ACCURACY_DECIMALS)

    await cg.register_component(var, config)

    if pc_typ:
        cg.add(var.set_component_source(f"tion_{ctyp}_{pc_typ}"))
    else:
        cg.add(var.set_component_source(f"tion_{ctyp}"))

    return var


# CONFIG_SCHEMA = (
#     cv.Schema(
#         {
#             cv.GenerateID(): cv.declare_id(Tion4sApiComponent),
#             cv.GenerateID(CONF_TION_API_BASE_ID): cv.declare_id(Tion4sApi),
#             cv.GenerateID(CONF_TION_API_ID): cv.declare_id(TionVPortApi),
#         }
#     )
#     .extend(vport.VPORT_CLIENT_SCHEMA)
#     .extend(cv.polling_component_schema("60s"))
# )
