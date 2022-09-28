import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client
from esphome.cpp_generator import MockObjClass
from .. import vport, tion  # pylint: disable=relative-beyond-top-level
from esphome.const import CONF_ID

CONF_STATE_TIMEOUT = "state_timeout"
CONF_PERSISTENT_CONNECTION = "persistent_connection"
CONF_PROTOCOL_ID = "protocol_id"

TionBLEVPort = tion.tion_ns.class_("TionBLEVPort", cg.PollingComponent, vport.VPort)


def tion_ble_schema(vport_class: MockObjClass, protocol_class: MockObjClass):
    return (
        vport.vport_schema(vport_class.template(protocol_class), "60s")
        .extend(ble_client.BLE_CLIENT_SCHEMA)
        .extend(
            {
                cv.GenerateID(CONF_PROTOCOL_ID): cv.declare_id(protocol_class),
                cv.Optional(
                    CONF_STATE_TIMEOUT, default="15s"
                ): cv.positive_time_period_milliseconds,
                cv.Optional(CONF_PERSISTENT_CONNECTION, default=False): cv.boolean,
            }
        )
    )


async def setup_tion_ble(config):
    prot = cg.new_Pvariable(config[CONF_PROTOCOL_ID])
    var = cg.new_Pvariable(config[CONF_ID], prot)
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT]))
    cg.add(var.set_persistent_connection(config[CONF_PERSISTENT_CONNECTION]))
    return var
