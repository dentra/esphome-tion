import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass
from .. import tion, vport  # pylint: disable=relative-beyond-top-level
from ..vport import setup_vport_ble
from esphome.const import CONF_ID

CONF_STATE_TIMEOUT = "state_timeout"
CONF_PROTOCOL_ID = "protocol_id"

TionBLEVPort = tion.tion_ns.class_("TionBLEVPort", cg.PollingComponent, vport.VPort)


def tion_ble_schema(vport_class: MockObjClass, protocol_class: MockObjClass):
    return vport.vport_ble_schema(
        vport_class.template(protocol_class), protocol_class, "60s"
    ).extend(
        {
            cv.Optional(
                CONF_STATE_TIMEOUT, default="15s"
            ): cv.positive_time_period_milliseconds,
        }
    )


async def setup_tion_ble(config):
    var = await setup_vport_ble(config)
    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    return var
