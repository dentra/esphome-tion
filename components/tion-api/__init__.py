import logging
from pathlib import Path

from esphome.core import CORE

from ..tion import BREEZER_TYPES

_LOGGER = logging.getLogger(__name__)

TION_ALL_TYPES = [*BREEZER_TYPES.keys(), "iq"]
TION_ALL_PORTS = ["uart", "ble"]


def FILTER_SOURCE_FILES() -> list[str]:
    files_to_filter: list[str] = ["tion-api-firmware.h"]

    included_types: list[str] = []
    excluded_types: list[str] = list(TION_ALL_TYPES)
    configured_types = []
    for tion in CORE.config.get("tion", []):
        types = tion["type"]
        configured_types.extend(types if isinstance(types, list) else [types])
    # tion_rc emulates a breezer over BLE and needs its protocol even without a matching tion
    rc_config = CORE.config.get("tion_rc")
    if rc_config:
        configured_types.append(rc_config["type"])
    for typ in configured_types:
        if typ not in included_types:
            included_types.append(typ)
        if typ in excluded_types:
            excluded_types.remove(typ)

    included_ports: list[str] = [
        define.name[10:].lower()
        for define in CORE.defines
        if define.name in [f"USE_VPORT_{prt.upper()}" for prt in TION_ALL_PORTS]
    ]
    if rc_config and "ble" not in included_ports:
        included_ports.append("ble")
    excluded_ports: list[str] = list(TION_ALL_PORTS)
    for port in included_ports:
        excluded_ports.remove(port)

    _LOGGER.debug("included types: %s", included_types)
    _LOGGER.debug("excluded types: %s", excluded_types)
    _LOGGER.debug("included ports: %s", included_ports)
    _LOGGER.debug("excluded ports: %s", excluded_ports)

    for entry in Path(__file__).parent.iterdir():
        for typ in excluded_types:
            suffixes = filter(
                None,
                [
                    f"-{typ}.cpp",
                    f"-{typ}.h",
                    f"-{typ}-internal.h",
                ],
            )
            for suffix in suffixes:
                if entry.name.endswith(suffix):
                    if (
                        "4s" in included_types
                        and typ == "lt"
                        and entry.name.startswith("tion-api-ble-lt.")
                    ):
                        pass  # tion 4s ble requires lt ble
                    else:
                        files_to_filter.append(entry.name)
        for prt in excluded_ports:
            for typ in BREEZER_TYPES.keys():
                suffixes = [
                    f"-{prt}-{typ}.cpp",
                    f"-{prt}-{typ}.h",
                    f"tion-api-{prt}.h",
                ]
                for suffix in suffixes:
                    if entry.name.endswith(suffix):
                        files_to_filter.append(entry.name)

    if "-DTION_ENABLE_PI_CONTROLLER" not in CORE.build_flags:
        files_to_filter.append("pi_controller.cpp")
        files_to_filter.append("pi_controller.h")

    _LOGGER.debug("filtered files: %s", set(files_to_filter))
    return set(files_to_filter)
