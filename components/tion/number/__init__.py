import dataclasses
from typing import Optional

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_INITIAL_VALUE,
    CONF_MODE,
    CONF_RESTORE_VALUE,
    CONF_UNIT_OF_MEASUREMENT,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS,
    UNIT_MINUTE,
    UNIT_SECOND,
)

from .. import cgp, new_pc, tion_ns

TionNumber = tion_ns.class_("TionNumber", number.Number, cg.Component)

CONF_TRAITS = "_traits"


@dataclasses.dataclass
class Traits:
    step: Optional[float] = 1
    min_value: Optional[float] = float("NaN")
    max_value: Optional[float] = float("NaN")
    initial_value: Optional[float] = None

    def get_initial_value(self, config: dict) -> Optional[float]:
        value = config.get(CONF_INITIAL_VALUE, self.initial_value)
        if not isinstance(value, cv.TimePeriod):
            return value
        unit_of_measurement = config.get(CONF_UNIT_OF_MEASUREMENT, "")
        if unit_of_measurement == UNIT_MINUTE:
            return value.minutes
        if unit_of_measurement == UNIT_SECOND:
            return value.seconds
        return value.milliseconds


PC = new_pc(
    {
        "fan_speed": {
            CONF_ICON: cgp.ICON_FAN,
        },
        "target_temperature": {
            CONF_ICON: cgp.ICON_THERMOMETER_AUTO,
            CONF_UNIT_OF_MEASUREMENT: UNIT_CELSIUS,
        },
        "boost_time": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_TIMER_COG,
            CONF_UNIT_OF_MEASUREMENT: UNIT_MINUTE,
            CONF_TRAITS: Traits(
                min_value=1,
                max_value=60,
                initial_value=10,
            ),
        },
        "auto_setpoint": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_FAN_AUTO,
            CONF_TRAITS: Traits(
                step=10,
                initial_value=600,
            ),
        },
        "auto_min_fan_speed": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_FAN_CHEVRON_DOWN,
            CONF_TRAITS: Traits(
                initial_value=1,
            ),
        },
        "auto_max_fan_speed": {
            CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
            CONF_ICON: cgp.ICON_FAN_CHEVRON_UP,
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
)


CONFIG_SCHEMA = PC.number_schema(
    TionNumber,
    {
        cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(
            number.NUMBER_MODES, upper=True
        ),
        cv.Optional(CONF_INITIAL_VALUE): cv.Any(
            cv.float_, cv.positive_time_period_milliseconds
        ),
        cv.Optional(CONF_RESTORE_VALUE): cv.boolean,
    },
)


async def to_code(config: dict):
    traits = PC.get_info_prop(config, CONF_TRAITS, Traits())

    var = await PC.new_number(
        config,
        min_value=traits.min_value,
        max_value=traits.max_value,
        step=traits.step,
    )

    cgp.setup_value(config, CONF_RESTORE_VALUE, var.set_restore_value)
    if initial_value := traits.get_initial_value(config):
        cg.add(var.set_initial_value(initial_value))
