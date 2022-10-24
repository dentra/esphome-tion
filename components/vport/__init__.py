import esphome.codegen as cg
from esphome.components import ble_client, uart
import esphome.config_validation as cv
from esphome import core
from esphome.cpp_generator import MockObjClass
from esphome.const import CONF_ID

AUTO_LOAD = ["etl"]

IS_PLATFORM_COMPONENT = True

CONF_VPORT_ID = "vport_id"
CONF_PERSISTENT_CONNECTION = "persistent_connection"
CONF_DISABLE_SCAN = "disable_scan"
CONF_PROTOCOL_ID = "protocol_id"

vport_ns = cg.esphome_ns.namespace("vport")
VPort = vport_ns.class_("VPort")

VPORT_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_VPORT_ID): cv.use_id(VPort),
    }
)


def vport_schema(
    vport_class: MockObjClass, protocol_class: MockObjClass, default_update_interval
):
    if not vport_class.inherits_from(VPort):
        raise cv.Invalid("Not a VPort Component")
    return cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(vport_class),
            cv.GenerateID(CONF_PROTOCOL_ID): cv.declare_id(protocol_class),
        }
    ).extend(cv.polling_component_schema(default_update_interval))


def vport_ble_schema(
    vport_class: MockObjClass,
    protocol_class: MockObjClass,
    default_update_interval="60s",
):
    return (
        vport_schema(vport_class, protocol_class, default_update_interval)
        .extend(ble_client.BLE_CLIENT_SCHEMA)
        .extend(
            {
                cv.Optional(CONF_PERSISTENT_CONNECTION, default=False): cv.boolean,
                cv.Optional(CONF_DISABLE_SCAN, default=False): cv.boolean,
            }
        )
    )


def vport_uart_schema(
    vport_class: MockObjClass,
    protocol_class: MockObjClass,
    default_update_interval="30s",
):
    return vport_schema(vport_class, protocol_class, default_update_interval).extend(
        uart.UART_DEVICE_SCHEMA
    )


async def vport_get_var(config):
    return await cg.get_variable(config[CONF_VPORT_ID])


async def setup_vport_ble(config):
    prot = cg.new_Pvariable(config[CONF_PROTOCOL_ID])
    var = cg.new_Pvariable(config[CONF_ID], prot)
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    cg.add(var.set_persistent_connection(config[CONF_PERSISTENT_CONNECTION]))
    cg.add(var.set_disable_scan(config[CONF_DISABLE_SCAN]))
    return var


async def setup_vport_uart(config):
    uart_parent = await cg.get_variable(config[uart.CONF_UART_ID])
    prot = cg.new_Pvariable(config[CONF_PROTOCOL_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_parent, prot)
    await cg.register_component(var, config)
    return var


async def to_code(config):
    # add_define wont work for esp32
    if "uart" in core.CORE.config:
        cg.add_build_flag("-DUSE_VPORT_UART")
    if "esp32_ble_tracker" in core.CORE.config:
        cg.add_build_flag("-DUSE_VPORT_BLE")
