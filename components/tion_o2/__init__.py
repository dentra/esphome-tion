import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.cpp_generator import MockObjClass

# pylint: disable-next=relative-beyond-top-level
from .. import tion

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["tion"]


TionO2Api = tion.tion_ns.class_("TionO2Api")


def tion_o2_schema(tion_class: MockObjClass, tion_base_schema: cv.Schema):
    return tion.tion_schema(tion_class, TionO2Api, tion_base_schema)


async def setup_tion_o2(config, compoent_reg):
    """Code generation entry point"""
    # pylint: disable-next=unused-variable
    var = await tion.setup_tion_core(config, compoent_reg)
    cg.add_build_flag("-DTION_MAX_FAN_SPEED=4")
