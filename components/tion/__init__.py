import logging
from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv

# from esphome.components import binary_sensor as esphome_binary_sensor
# from esphome.components import button as esphome_button
# from esphome.components import number as esphome_number
# from esphome.components import select as esphome_select
# from esphome.components import text_sensor as esphome_text_sensor
from esphome.components import sensor as esphome_sensor
from esphome.components import switch as esphome_switch
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
    CONF_TEMPERATURE,
    CONF_TYPE,
    CONF_UNIT_OF_MEASUREMENT,
)
from esphome.core import ID
from esphome.cpp_generator import MockObjClass

from .. import vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["etl", "tion-api"]

_LOGGER = logging.getLogger(__name__)

CONF_TION_API_BASE_ID = "tion_api_base_id"
CONF_TION_API_ID = "tion_api_id"
CONF_TION_COMPONENT_CLASS = "tion_component_class"

CONF_PRESETS = "presets"
CONF_PRESET_FAN_SPEED = "fan_speed"
CONF_PRESET_TEMPERATURE = CONF_TEMPERATURE
CONF_PRESET_GATE_POSITION = "gate_position"
CONF_BUTTON_PRESETS = "button_presets"

CONF_STATE_TIMEOUT = "state_timeout"
CONF_STATE_WARNOUT = "state_warnout"
CONF_BATCH_TIMEOUT = "batch_timeout"


tion_ns = cg.esphome_ns.namespace("tion")
TionVPortApi = tion_ns.class_("TionVPortApi")
TionApiComponent = tion_ns.class_("TionApiComponent", cg.Component)

dentra_tion_ns = cg.global_ns.namespace("dentra").namespace("tion")
TionGatePosition = dentra_tion_ns.namespace("TionGatePosition")

BREEZER_TYPES = {
    "o2": tion_ns.class_("TionO2ApiComponent", TionApiComponent),
    "3s": tion_ns.class_("Tion3sApiComponent", TionApiComponent),
    "4s": tion_ns.class_("Tion4sApiComponent", TionApiComponent),
    "lt": tion_ns.class_("TionLtApiComponent", TionApiComponent),
}

PRESET_GATE_POSITIONS = {
    "none": TionGatePosition.NONE,
    "outdoor": TionGatePosition.OUTDOOR,
    "indoor": TionGatePosition.INDOOR,
    "mixed": TionGatePosition.MIXED,
}


CUSTOM_PRESET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_POWER): cv.boolean,
        cv.Optional(CONF_HEATER): cv.boolean,
        cv.Optional(CONF_PRESET_FAN_SPEED, default=0): cv.int_range(min=0, max=6),
        cv.Optional(CONF_PRESET_TEMPERATURE, default=0): cv.int_range(min=-30, max=30),
        cv.Optional(CONF_PRESET_GATE_POSITION, default="none"): cv.one_of(
            *PRESET_GATE_POSITIONS, lower=True
        ),
    }
)

BUTTON_PRESETS_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PRESET_TEMPERATURE): cv.All(
            cv.ensure_list(cv.int_range(min=1, max=25)), cv.Length(min=3, max=3)
        ),
        cv.Required(CONF_PRESET_FAN_SPEED): cv.All(
            cv.ensure_list(cv.int_range(min=1, max=6)), cv.Length(min=3, max=3)
        ),
    }
)


def check_type(key, typ):
    def validator(config):
        if key in config and config[CONF_TYPE] != typ:
            raise cv.Invalid(f"{key} is not valid for the type {typ}")
        return config

    return validator


# async def setup_binary_sensor(config: dict, key: str, setter):
#     """Setup binary sensor"""
#     if key not in config:
#         return None
#     sens = await esphome_binary_sensor.new_binary_sensor(config[key])
#     cg.add(setter(sens))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return sens


async def setup_switch(config: dict, key: str, setter, parent):
    """Setup switch"""
    if key not in config:
        return None
    swch = await esphome_switch.new_switch(config[key], parent)
    cg.add(setter(swch))
    cg.add_define(f"USE_TION_{key.upper()}")
    return swch


# async def setup_sensor(config: dict, key: str, setter):
#     """Setup sensor"""
#     if key not in config:
#         return None
#     sens = await esphome_sensor.new_sensor(config[key])
#     cg.add(setter(sens))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return sens


# async def setup_text_sensor(config: dict, key: str, setter):
#     """Setup text sensor"""
#     if key not in config:
#         return None
#     sens = await esphome_text_sensor.new_text_sensor(config[key])
#     cg.add(setter(sens))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return sens


# async def setup_number(
#     config: dict, key: str, setter, min_value: int, max_value: int, step: int
# ):
#     """Setup number"""
#     if key not in config:
#         return None
#     numb = await esphome_number.new_number(
#         config[key], min_value=min_value, max_value=max_value, step=step
#     )
#     cg.add(setter(numb))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return numb


# async def setup_select(config: dict, key: str, setter, parent, options):
#     """Setup select"""
#     if key not in config:
#         return None
#     conf = config[key]
#     slct = cg.new_Pvariable(conf[CONF_ID], parent)
#     await esphome_select.register_select(slct, conf, options=options)
#     cg.add(setter(slct))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return slct


# async def setup_button(config: dict, key: str, setter, parent):
#     """Setup button"""
#     if key not in config:
#         return None
#     butn = await esphome_button.new_button(config[key], parent)
#     cg.add(setter(butn))
#     cg.add_define(f"USE_TION_{key.upper()}")
#     return butn


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
    paren = await cg.get_variable(api)
    if props and CONF_TION_COMPONENT_CLASS not in props:
        var = await ctor(config, cg.TemplateArguments(get_pc_class()), paren, **kwargs)
    else:
        var = await ctor(config, paren, **kwargs)

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


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TionApiComponent),
            cv.Required(CONF_TYPE): cv.one_of(*BREEZER_TYPES, lower=True),
            cv.Optional(CONF_BUTTON_PRESETS): BUTTON_PRESETS_SCHEMA,
            cv.GenerateID(CONF_TION_API_ID): cv.declare_id(TionVPortApi),
            cv.Optional(CONF_STATE_TIMEOUT, default="3s"): cv.update_interval,
            cv.Optional(
                CONF_BATCH_TIMEOUT, default="200ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FORCE_UPDATE): cv.boolean,
            cv.Optional(CONF_PRESETS): cv.Schema(
                {cv.string_strict: CUSTOM_PRESET_SCHEMA}
            ),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("15s")),
    check_type(CONF_BUTTON_PRESETS, "lt"),
)


async def _setup_tion_api(config: dict):
    component_class: MockObjClass = BREEZER_TYPES[config[CONF_TYPE]]

    # get vport instance
    prt = await vport.vport_get_var(config)
    # create TionVPortApi wrapper
    api = cg.new_Pvariable(
        config[CONF_TION_API_ID],
        cg.TemplateArguments(
            vport.vport_find(config).type.class_("frame_spec_type"),
            component_class.class_("Api"),
        ),
        prt,
    )
    cg.add(prt.set_api(api))

    component_id: ID = config[CONF_ID]
    component_id.type = component_class
    var = cg.new_Pvariable(config[CONF_ID], api, prt.get_type())
    await cg.register_component(var, config)

    cg.add(var.set_component_source(f"tion_{config[CONF_TYPE]}"))

    # cg.add_library("tion-api", None, "https://github.com/dentra/tion-api")
    cg.add_build_flag("-DTION_ESPHOME")

    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    cg.add(var.set_batch_timeout(config[CONF_BATCH_TIMEOUT]))
    if CONF_FORCE_UPDATE in config:
        cg.add(var.set_force_update(config[CONF_FORCE_UPDATE]))

    return var


def _setup_tion_api_presets(config: dict, var: cg.MockObj):
    if CONF_PRESETS not in config:
        return

    presets = set()
    for preset in config[CONF_PRESETS]:
        preset_name = str(preset).strip()
        if preset_name.lower() == "none":
            _LOGGER.warning("Preset 'none' is reserved")
            continue
        if preset_name.lower() in presets:
            _LOGGER.warning("Preset '%s' is already exists", preset)
            continue
        preset = config[CONF_PRESETS][preset_name]
        cg.add(
            var.add_preset(
                preset_name,
                cg.StructInitializer(
                    "",
                    ("target_temperature", preset[CONF_PRESET_TEMPERATURE]),
                    (
                        "heater_state",
                        int(preset[CONF_HEATER]) if CONF_HEATER in preset else -1,
                    ),
                    (
                        "power_state",
                        int(preset[CONF_POWER]) if CONF_POWER in preset else -1,
                    ),
                    ("fan_speed", preset[CONF_PRESET_FAN_SPEED]),
                    (
                        "gate_position",
                        PRESET_GATE_POSITIONS[preset[CONF_PRESET_GATE_POSITION]],
                    ),
                ),
            )
        )

        presets.add(preset_name)


def _setup_tion_api_button_presets(config: dict, var: cg.MockObj):
    if CONF_BUTTON_PRESETS not in config:
        return
    button_presets = config[CONF_BUTTON_PRESETS]

    cg.add(
        var.set_button_presets(
            cg.StructInitializer(
                "",
                ("tmp", button_presets[CONF_TEMPERATURE]),
                ("fan", button_presets[CONF_PRESET_FAN_SPEED]),
            )
        )
    )


async def to_code(config: dict):
    var = await _setup_tion_api(config)
    _setup_tion_api_presets(config, var)
    _setup_tion_api_button_presets(config, var)
