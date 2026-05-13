import logging
from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, core
from esphome.automation import register_action, register_condition
from esphome.components import sensor as esphome_sensor
from esphome.const import (
    CONF_CO2,
    CONF_FORCE_UPDATE,
    CONF_HEATER,
    CONF_ID,
    CONF_LAMBDA,
    CONF_ON_STATE,
    CONF_POWER,
    CONF_RESTORE_STATE,
    CONF_TEMPERATURE,
    CONF_TYPE,
    CONF_VALUE,
)
from esphome.const import (
    __version__ as ESPHOME_VERSION,
)
from esphome.core import ID
from esphome.cpp_generator import MockObjClass

from .. import cgp, vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion-api", "cgp"]

CONF_TION_ID = "tion_id"
CONF_COMPONENT_CLASS = cgp.CONF_COMPONENT_CLASS

CONF_PRESETS = "presets"
CONF_FAN_SPEED = "fan_speed"
CONF_GATE_POSITION = "gate_position"
CONF_AUTO = "auto"
CONF_BUTTON_PRESETS = "button_presets"
CONF_ENABLE_KIV = "enable_kiv"
CONF_WORK_TIME = "work_time"

CONF_STATE_TIMEOUT = "state_timeout"
CONF_STATE_WARNOUT = "state_warnout"
CONF_BATCH_TIMEOUT = "batch_timeout"

CONF_AUTO_CO2 = CONF_CO2
CONF_AUTO_SETPOINT = "setpoint"
CONF_AUTO_MIN_FAN_SPEED = f"min_{CONF_FAN_SPEED}"
CONF_AUTO_MAX_FAN_SPEED = f"max_{CONF_FAN_SPEED}"
CONF_AUTO_PI_CONTROLLER = "pi_controller"
CONF_AUTO_KP = "kp"
CONF_AUTO_TI = "ti"
CONF_AUTO_DB = "db"

tion_ns = cg.esphome_ns.namespace("tion")
dentra_tion_ns = cg.global_ns.namespace("dentra").namespace("tion")

TionVPortApi = tion_ns.class_("TionVPortApi")
TionApiComponent = tion_ns.class_("TionApiComponent", cg.Component)

TionStateRef = dentra_tion_ns.namespace("TionState").operator("ref").operator("const")
TionGatePosition = dentra_tion_ns.namespace("TionGatePosition")

StateTrigger = tion_ns.class_("StateTrigger", automation.Trigger.template(TionStateRef))

BREEZER_TYPES = {
    "o2": tion_ns.class_("TionO2ApiComponent", TionApiComponent),
    "3s": tion_ns.class_("Tion3sApiComponent", TionApiComponent),
    "4s": tion_ns.class_("Tion4sApiComponent", TionApiComponent),
    "lt": tion_ns.class_("TionLtApiComponent", TionApiComponent),
}

PRESET_GATE_POSITIONS = {
    "outdoor": TionGatePosition.OUTDOOR,
    "indoor": TionGatePosition.INDOOR,
    "mixed": TionGatePosition.MIXED,
}


PRESET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_POWER, default=True): cv.Any(cv.none, cv.boolean),
        cv.Optional(CONF_HEATER, default="none"): cv.Any(cv.none, cv.boolean),
        cv.Optional(CONF_FAN_SPEED, default="none"): cv.Any(
            cv.none, cv.int_range(min=0, max=6)
        ),
        cv.Optional(CONF_TEMPERATURE, default="none"): cv.Any(
            cv.none, cv.int_range(min=0, max=30)
        ),
        cv.Optional(CONF_GATE_POSITION, default="none"): cv.Any(
            cv.none, cv.enum(PRESET_GATE_POSITIONS, lower=True)
        ),
        cv.Optional(CONF_AUTO, default="none"): cv.Any(cv.none, cv.boolean),
        cv.Optional(CONF_WORK_TIME, default="none"): cv.Any(
            cv.All(
                cv.positive_time_period_seconds,
                cv.Range(
                    min=cv.TimePeriod(minutes=1),
                    max=cv.TimePeriod(minutes=60),
                ),
            ),
            cv.none,
        ),
    }
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

AUTO_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_AUTO_CO2): cv.Any(
            cv.use_id(esphome_sensor.Sensor), cv.boolean
        ),
        cv.Optional(CONF_AUTO_SETPOINT): cv.int_range(500, 1400),
        cv.Inclusive(CONF_AUTO_MIN_FAN_SPEED, "auto_fan_speed"): cv.int_range(0, 5),
        cv.Inclusive(CONF_AUTO_MAX_FAN_SPEED, "auto_fan_speed"): cv.int_range(1, 6),
        cv.Exclusive(CONF_AUTO_PI_CONTROLLER, "auto_mode"): cv.Any(
            {
                cv.Optional(CONF_AUTO_KP, default=0.2736): cv.float_range(min=0.001),
                cv.Optional(CONF_AUTO_TI, default=8): cv.float_range(min=0.001),
                cv.Optional(CONF_AUTO_DB, default=20): cv.int_range(-100, 100),
            },
            None,
        ),
        cv.Exclusive(CONF_LAMBDA, "auto_mode"): cv.returning_lambda,
    }
)


def check_type(key, typ, required: bool = False):
    return cgp.validate_type(key, typ, required)


CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(TionApiComponent),
                cv.GenerateID(CONF_TION_ID): cv.declare_id(TionVPortApi),
                cv.Required(CONF_TYPE): cv.one_of(*BREEZER_TYPES, lower=True),
                cv.Optional(CONF_BUTTON_PRESETS): BUTTON_PRESETS_SCHEMA,
                cv.Optional(CONF_STATE_TIMEOUT, default="3s"): cv.update_interval,
                cv.Optional(
                    CONF_BATCH_TIMEOUT, default="200ms"
                ): cv.positive_time_period_milliseconds,
                cv.Optional(CONF_FORCE_UPDATE): cv.boolean,
                cv.Optional(CONF_PRESETS): cv.Schema({cv.string_strict: PRESET_SCHEMA}),
                cv.Optional(CONF_ON_STATE): cgp.automation_schema(StateTrigger),
                cv.Optional(CONF_AUTO): AUTO_SCHEMA,
                cv.Optional(CONF_RESTORE_STATE, default=False): cv.boolean,
                cv.Optional(CONF_ENABLE_KIV, default=False): cv.boolean,
            }
        )
        .extend(vport.VPORT_CLIENT_SCHEMA)
        .extend(cv.polling_component_schema("60s")),
        cgp.validate_type(CONF_BUTTON_PRESETS, "lt"),
    ),
)


async def new_vport_api_wrapper(config: dict, component_class: MockObjClass):
    # get vport instance
    prt = await vport.vport_get_var(config)
    # create TionVPortApi wrapper
    api = cg.new_Pvariable(
        config[CONF_TION_ID],
        cg.TemplateArguments(
            vport.vport_find(config).type.class_("frame_spec_type"),
            component_class.class_("Api"),
        ),
        prt,
    )
    return prt, api


async def _setup_tion_api(config: dict):
    component_class: MockObjClass = BREEZER_TYPES[config[CONF_TYPE]]

    prt, api = await new_vport_api_wrapper(config, component_class)
    cg.add(prt.set_api(api))

    component_id: ID = config[CONF_ID]
    component_id.type = component_class
    var = cg.new_Pvariable(config[CONF_ID], api, prt.get_type())
    await cg.register_component(var, config)

    component_source = f"tion[type={config[CONF_TYPE]}]"

    if cv.Version.parse(ESPHOME_VERSION) >= cv.Version.parse("2026.4.0"):
        from esphome.cpp_helpers import register_component_source

        idx = register_component_source(component_source)
        cg.add(var.set_component_source_(idx))
    else:
        if cv.Version.parse(ESPHOME_VERSION) >= cv.Version.parse("2025.9.0"):
            from esphome.cpp_generator import LogStringLiteral

            component_source = LogStringLiteral(component_source)
        cg.add(var.set_component_source(component_source))

    # cg.add_library("tion-api", None, "https://github.com/dentra/tion-api")
    cg.add_build_flag("-DTION_ESPHOME")

    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    cg.add(var.set_batch_timeout(config[CONF_BATCH_TIMEOUT]))
    cgp.setup_value(config, CONF_FORCE_UPDATE, var.set_force_update)

    return var


def _setup_tion_api_presets(config: dict, var: cg.MockObj):
    if CONF_PRESETS not in config:
        return

    presets = {}
    for preset in config[CONF_PRESETS]:
        preset_name = str(preset).strip()
        if preset_name.lower() == "none":
            logging.warning('Preset "none" is reserved')
            continue

        preset = config[CONF_PRESETS][preset_name]
        for k, v in presets.items():
            if preset == v:
                logging.warning('Preset "%s" duplicates "%s"', preset_name, k)

        args = [
            ("target_temperature", CONF_TEMPERATURE),
            ("heater_state", CONF_HEATER),
            ("power_state", CONF_POWER),
            ("fan_speed", CONF_FAN_SPEED),
            ("gate_position", CONF_GATE_POSITION),
            ("auto_state", CONF_AUTO),
            ("work_time", CONF_WORK_TIME),
        ]
        for arg in args:
            if preset[arg[1]] is not None:
                break
        else:
            logging.warning(
                'Preset "%s" completely empty and do not change device state',
                preset_name,
            )

        def preset_arg(nm: str):
            if preset[nm] is not None:
                return preset[nm]
            if nm == CONF_GATE_POSITION:
                return TionGatePosition.UNKNOWN
            if nm == CONF_WORK_TIME:
                return 0
            return -1

        struct_args = [(arg[0], preset_arg(arg[1])) for arg in args]
        cg.add(var.add_preset(preset_name, cg.StructInitializer("", *struct_args)))

        presets[preset_name] = preset


def _setup_tion_api_button_presets(config: dict, var: cg.MockObj):
    if CONF_BUTTON_PRESETS not in config:
        return
    button_presets = config[CONF_BUTTON_PRESETS]

    struct_args = [
        ("tmp", button_presets[CONF_TEMPERATURE]),
        ("fan", button_presets[CONF_FAN_SPEED]),
    ]

    cg.add(var.set_button_presets(cg.StructInitializer("", *struct_args)))


async def _setup_auto(config: dict, var):
    api = var.Papi()

    if not isinstance(config[CONF_AUTO_CO2], bool):
        code = f"{var}->auto_update(x);"
        lam = await cg.process_lambda(
            value=core.Lambda(code), parameters=[(cg.float_, "x")], capture=""
        )
        cg.add(cg.MockObj(config[CONF_AUTO_CO2].id, "->").add_on_state_callback(lam))

    cgp.setup_value(config, CONF_AUTO_SETPOINT, api.set_auto_setpoint)
    cgp.setup_value(config, CONF_AUTO_MIN_FAN_SPEED, api.set_auto_min_fan_speed)
    cgp.setup_value(config, CONF_AUTO_MAX_FAN_SPEED, api.set_auto_max_fan_speed)

    if CONF_AUTO_PI_CONTROLLER in config:
        cgp.setup_values(
            config[CONF_AUTO_PI_CONTROLLER] or {},
            [CONF_AUTO_KP, CONF_AUTO_TI, CONF_AUTO_DB],
            api.set_auto_pi_data,
        )
        cg.add_build_flag("-DTION_ENABLE_PI_CONTROLLER")

    await cgp.setup_lambda(
        config,
        CONF_LAMBDA,
        api.set_auto_update_func,
        parameters=[(cg.uint16, "x")],
        return_type=cg.uint8,
    )


async def to_code(config: dict):
    for conf in config:
        var = await _setup_tion_api(conf)
        _setup_tion_api_presets(conf, var)
        _setup_tion_api_button_presets(conf, var)
        await cgp.setup_automation(conf, CONF_ON_STATE, var, (TionStateRef, "x"))
        if CONF_AUTO in conf:
            await _setup_auto(conf[CONF_AUTO], var)
        if conf[CONF_RESTORE_STATE]:
            cg.add_define("USE_TION_RESTORE_STATE")
            cg.add(var.set_rtc_key(f"{conf[CONF_ID].id}-v2"))
        if CONF_ENABLE_KIV:
            cg.add_build_flag("-DTION_ENABLE_KIV")


def new_pc(pc_cfg: dict[str, str | dict[str, Any]]):
    def get_component_type() -> str:
        import inspect

        stack = inspect.stack()
        frame = stack[2][0]
        ctyp = inspect.getmodule(frame).__name__.split(".")[-1:][0]

        return ctyp

    class TionPC(cgp.PC):
        def get_type_component(self, config: dict):
            cls = super().get_type_component(config)
            if cls and not isinstance(cls, cg.MockObjClass):
                import copy

                # creates same object class as base entity
                cpy: cg.MockObjClass = copy.deepcopy(config[CONF_ID].type)
                cls = str(cls)
                # additionally copy ns
                if "::" not in cls:
                    cls = "::".join(str(cpy.base).split("::")[:-1] + [cls])
                cpy.base = cls
                cls = cpy
            return cls

        def get_type_class(self, config: dict):
            ct_cls = self.get_type_component(config)
            # skip pc components with declared classes
            if ct_cls:
                return None
            pc_typ = self.get_type(config)
            # skip non pc components e.g. fan and climate
            if not pc_typ:
                return None
            ct_typ = self.component_type
            # special case for the swithc namespace
            if ct_typ == "switch":
                ct_typ = f"{ct_typ}_"
            pc_typ = pc_typ.replace("_", " ").title().replace(" ", "")
            return tion_ns.class_(f"property_controller::{ct_typ}::{pc_typ}")

    return TionPC(TionApiComponent, CONF_TION_ID, pc_cfg, get_component_type())


TION_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.Required(CONF_ID): cv.use_id(TionApiComponent)}
)

PowerToggleAction = tion_ns.class_("PowerToggleAction", automation.Action)
PowerTurnOnAction = tion_ns.class_("PowerTurnOnAction", automation.Action)
PowerTurnOffAction = tion_ns.class_("PowerTurnOnAction", automation.Action)
PowerCondition = tion_ns.class_("PowerCondition", automation.Condition)

HeaterToggleAction = tion_ns.class_("HeaterToggleAction", automation.Action)
HeaterTurnOnAction = tion_ns.class_("HeaterTurnOnAction", automation.Action)
HeaterTurnOffAction = tion_ns.class_("HeaterTurnOffAction", automation.Action)
HeaterCondition = tion_ns.class_("HeaterCondition", automation.Condition)

BoostToggleAction = tion_ns.class_("BoostToggleAction", automation.Action)
BoostTurnOnAction = tion_ns.class_("BoostTurnOnAction", automation.Action)
BoostTurnOffAction = tion_ns.class_("BoostTurnOffAction", automation.Action)
BoostCondition = tion_ns.class_("BoostCondition", automation.Condition)

AutoToggleAction = tion_ns.class_("AutoToggleAction", automation.Action)
AutoTurnOnAction = tion_ns.class_("AutoTurnOnAction", automation.Action)
AutoTurnOffAction = tion_ns.class_("AutoTurnOffAction", automation.Action)
AutoCondition = tion_ns.class_("AutoCondition", automation.Condition)

FanSpeedSetAction = tion_ns.class_("FanSpeedSetAction", automation.Action)


@register_action(
    "tion.power.toggle", PowerToggleAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.power.turn_on", PowerTurnOnAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.power.turn_off", PowerTurnOffAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.heater.toggle", HeaterToggleAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.heater.turn_on", HeaterTurnOnAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.heater.turn_off", HeaterTurnOffAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.boost.toggle", BoostToggleAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.boost.turn_on", BoostTurnOnAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.boost.turn_off", BoostTurnOffAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.auto.toggle", AutoToggleAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.auto.turn_on", AutoTurnOnAction, TION_ACTION_SCHEMA, synchronous=False
)
@register_action(
    "tion.auto.turn_off", AutoTurnOffAction, TION_ACTION_SCHEMA, synchronous=False
)
async def tion_switch_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@register_condition("tion.power.is_on", PowerCondition, TION_ACTION_SCHEMA)
@register_condition("tion.heater.is_on", HeaterCondition, TION_ACTION_SCHEMA)
@register_condition("tion.boost.is_on", BoostCondition, TION_ACTION_SCHEMA)
@register_condition("tion.auto.is_on", BoostCondition, TION_ACTION_SCHEMA)
async def tion_switch_is_on_to_code(config, condition_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, paren, True)


@register_condition("tion.power.is_off", PowerCondition, TION_ACTION_SCHEMA)
@register_condition("tion.heater.is_off", HeaterCondition, TION_ACTION_SCHEMA)
@register_condition("tion.boost.is_off", BoostCondition, TION_ACTION_SCHEMA)
@register_condition("tion.auto.is_off", BoostCondition, TION_ACTION_SCHEMA)
async def tion_switch_is_off_to_code(config, condition_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, paren, False)


TION_OPERATION_BASE_SCHEMA = cv.Schema(
    {cv.Required(CONF_ID): cv.use_id(TionApiComponent)}
)


@register_action(
    "tion.fan_speed.set",
    FanSpeedSetAction,
    TION_OPERATION_BASE_SCHEMA.extend(
        {cv.Required(CONF_VALUE): cv.templatable(cv.int_range(min=0, max=6))}
    ),
    synchronous=False,
)
async def tion_number_set_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_VALUE], args, int)
    cg.add(var.set_value(template_))
    return var
