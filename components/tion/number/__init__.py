from typing import Any

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INITIAL_VALUE,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_MODE,
    CONF_RESTORE_VALUE,
    CONF_STEP,
    CONF_UNIT_OF_MEASUREMENT,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS,
    UNIT_MINUTE,
    UNIT_SECOND,
)

from .. import get_pc_info, new_pc_component, pc_schema, tion_ns

TionNumber = tion_ns.class_("TionNumber", number.Number, cg.Component)

PROPERTIES = {
    "fan_speed": {
        CONF_ICON: "mdi:fan",
        "traits": {
            CONF_MIN_VALUE: 0,
        },
    },
    "target_temperature": {
        CONF_ICON: "mdi:thermometer-auto",
        "traits": {
            CONF_UNIT_OF_MEASUREMENT: UNIT_CELSIUS,
        },
    },
    "boost_time": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
        CONF_ICON: "mdi:timer-cog",
        "traits": {
            CONF_MIN_VALUE: 1,
            CONF_MAX_VALUE: 60,
            CONF_UNIT_OF_MEASUREMENT: UNIT_MINUTE,
            CONF_INITIAL_VALUE: 10,
        },
    },
    # aliases
    "fan": "fan_speed",
    "speed": "fan_speed",
    "boost": "boost_time",
}

_UNDEF = object()


NUMBER_SCHEMA_EXT = {
    cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(number.NUMBER_MODES, upper=True),
    cv.Optional(CONF_INITIAL_VALUE): cv.Any(
        cv.float_, cv.positive_time_period_milliseconds
    ),
    cv.Optional(CONF_RESTORE_VALUE): cv.boolean,
}


CONFIG_SCHEMA = pc_schema(
    number.number_schema(TionNumber).extend(NUMBER_SCHEMA_EXT), PROPERTIES
)


def _get_value(value, uom: str) -> float:
    if not isinstance(value, cv.TimePeriod):
        return value
    if uom == UNIT_MINUTE:
        return value.minutes
    if uom == UNIT_SECOND:
        return value.seconds
    return value.milliseconds


async def to_code(config: dict):
    (_, props) = get_pc_info(config, PROPERTIES)

    traits = props["traits"]

    var = await new_pc_component(
        config,
        number.new_number,
        PROPERTIES,
        min_value=traits[CONF_MIN_VALUE] if CONF_MIN_VALUE in traits else float("NaN"),
        max_value=traits[CONF_MAX_VALUE] if CONF_MAX_VALUE in traits else float("NaN"),
        step=traits[CONF_STEP] if CONF_STEP in traits else 1,
    )

    if CONF_UNIT_OF_MEASUREMENT in traits:
        cg.add(var.traits.set_unit_of_measurement(traits[CONF_UNIT_OF_MEASUREMENT]))

    if CONF_RESTORE_VALUE in config:
        cg.add(var.set_restore_value(config[CONF_RESTORE_VALUE]))

    if CONF_INITIAL_VALUE in config:
        cg.add(
            var.set_initial_value(
                _get_value(
                    config[CONF_INITIAL_VALUE],
                    traits.get(CONF_UNIT_OF_MEASUREMENT, ""),
                )
            )
        )
    elif CONF_INITIAL_VALUE in traits:
        cg.add(
            var.set_initial_value(
                _get_value(
                    traits[CONF_INITIAL_VALUE],
                    traits.get(CONF_UNIT_OF_MEASUREMENT, ""),
                )
            )
        )
