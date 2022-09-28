import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass

AUTO_LOAD = ["etl"]

IS_PLATFORM_COMPONENT = True

CONF_VPORT_ID = "vport_id"
vport_ns = cg.esphome_ns.namespace("vport")
VPort = vport_ns.class_("VPort")

VPORT_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_VPORT_ID): cv.use_id(VPort),
    }
)


def vport_schema(vport_class: MockObjClass, default_update_interval):
    if not vport_class.inherits_from(VPort):
        raise cv.Invalid("Not a VPort Component")
    return cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(vport_class),
        }
    ).extend(cv.polling_component_schema(default_update_interval))


async def vport_get_var(config):
    return await cg.get_variable(config[CONF_VPORT_ID])


# TODO setup update interval here
