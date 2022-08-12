from esphome.cpp_types import PollingComponent
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.core import ID
from esphome.components import climate, select, sensor
from esphome.const import (
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CUBIC_METER,
)
from .. import tion, vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "select"]

CONF_AIR_INTAKE = "air_intake"
CONF_PRODUCTIVITY = "productivity"
UNIT_CUBIC_METER_PER_HOUR = f"{UNIT_CUBIC_METER}/h"

Tion3s = tion.tion_ns.class_("Tion3s", PollingComponent, climate.Climate)
Tion3sAirIntakeSelect = tion.tion_ns.class_("Tion3sAirIntakeSelect", select.Select)

OPTIONS_AIR_INTAKE = ["Indoor", "Mixed", "Outdoor"]

CONFIG_SCHEMA = tion.tion_schema(Tion3s).extend(
    {
        cv.Optional(CONF_AIR_INTAKE): select.SELECT_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(Tion3sAirIntakeSelect),
                cv.Optional(CONF_ICON, default="mdi:air-conditioner"): cv.icon,
                cv.Optional(
                    CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG
                ): cv.entity_category,
            }
        ),
        cv.Optional(CONF_PRODUCTIVITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_CUBIC_METER_PER_HOUR,
            accuracy_decimals=2,
            icon="mdi:weather-windy",
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    """Code generation entry point"""
    var = await tion.setup_tion_core(config)
    await tion.setup_select(
        config, CONF_AIR_INTAKE, var.set_air_intake, var, OPTIONS_AIR_INTAKE
    )
    vport_parent = await vport.vport_get_var(config)
    cg.add(vport_parent.set_api(var))
    await tion.setup_sensor(config, CONF_PRODUCTIVITY, var.set_airflow_counter)
