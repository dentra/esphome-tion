from esphome.cpp_types import PollingComponent
import esphome.config_validation as cv
from esphome.components import climate, select, sensor
from esphome.const import (
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CUBIC_METER,
)
from .. import tion  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion", "select"]

CONF_AIR_INTAKE = "air_intake"
CONF_PRODUCTIVITY = "productivity"
UNIT_CUBIC_METER_PER_HOUR = f"{UNIT_CUBIC_METER}/h"

Tion3s = tion.tion_ns.class_("Tion3s", PollingComponent, climate.Climate)
TionApi3s = tion.tion_ns.class_("TionApi3s")
Tion3sAirIntakeSelect = tion.tion_ns.class_("Tion3sAirIntakeSelect", select.Select)

OPTIONS_AIR_INTAKE = ["Indoor", "Mixed", "Outdoor"]

CONFIG_SCHEMA = tion.tion_schema(Tion3s, TionApi3s).extend(
    {
        cv.Optional(CONF_AIR_INTAKE): select.select_schema(
            Tion3sAirIntakeSelect,
            icon="mdi:air-conditioner",
            entity_category=ENTITY_CATEGORY_CONFIG,
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
    # FIXME check and replace/remove
    # api = await cg.get_variable(config[tion.CONF_TION_API_ID])
    # prt = await vport.vport_get_var(config)
    # cg.add(prt.set_api(api))
    await tion.setup_sensor(config, CONF_PRODUCTIVITY, var.set_airflow_counter)
