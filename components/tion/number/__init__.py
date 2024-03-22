import dataclasses
from typing import Any, Optional, Union

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INITIAL_VALUE,
    CONF_MODE,
    CONF_RESTORE_VALUE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS,
    UNIT_MINUTE,
    UNIT_SECOND,
)

from .. import get_pc_info, new_pc_component, pc_schema, tion_ns

TionNumber = tion_ns.class_("TionNumber", number.Number, cg.Component)

CONF_TRAITS = "traits"


@dataclasses.dataclass
class Traits:
    step: Optional[float] = 1
    min_value: Optional[float] = float("NaN")
    max_value: Optional[float] = float("NaN")
    unit_of_measurement: Optional[str] = ""
    initial_value: Optional[float] = None

    def get_initial_value(self, config: dict) -> Optional[float]:
        value = config.get(CONF_INITIAL_VALUE, self.initial_value)
        if not isinstance(value, cv.TimePeriod):
            return value
        if self.unit_of_measurement == UNIT_MINUTE:
            return value.minutes
        if self.unit_of_measurement == UNIT_SECOND:
            return value.seconds
        return value.milliseconds


PROPERTIES = {
    "fan_speed": {
        CONF_ICON: "mdi:fan",
    },
    "target_temperature": {
        CONF_ICON: "mdi:thermometer-auto",
        CONF_TRAITS: Traits(
            unit_of_measurement=UNIT_CELSIUS,
        ),
    },
    "boost_time": {
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
        CONF_ICON: "mdi:timer-cog",
        CONF_TRAITS: Traits(
            min_value=1,
            max_value=60,
            unit_of_measurement=UNIT_MINUTE,
            initial_value=10,
        ),
    },
    "auto_setpoint": {
        CONF_ICON: "mdi:fan-auto",
        CONF_TRAITS: Traits(
            step=10,
            initial_value=600,
        ),
    },
    "auto_min_fan_speed": {
        CONF_ICON: "mdi:fan-chevron-down",
        CONF_TRAITS: Traits(
            initial_value=1,
        ),
    },
    "auto_max_fan_speed": {
        CONF_ICON: "mdi:fan-chevron-up",
        CONF_TRAITS: Traits(
            initial_value=3,
        ),
    },
    # aliases
    "fan": "fan_speed",
    "speed": "fan_speed",
    "boost": "boost_time",
    "auto_min_speed": "auto_min_fan_speed",
    "auto_max_speed": "auto_max_fan_speed",
}


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


async def to_code(config: dict):
    (_, props) = get_pc_info(config, PROPERTIES)

    traits = props.get(CONF_TRAITS, Traits())

    var = await new_pc_component(
        config,
        number.new_number,
        PROPERTIES,
        min_value=traits.min_value,
        max_value=traits.max_value,
        step=traits.step,
    )

    if traits.unit_of_measurement:
        cg.add(var.traits.set_unit_of_measurement(traits.unit_of_measurement))

    if CONF_RESTORE_VALUE in config:
        cg.add(var.set_restore_value(config[CONF_RESTORE_VALUE]))

    if initial_value := traits.get_initial_value(config):
        cg.add(var.set_initial_value(initial_value))
