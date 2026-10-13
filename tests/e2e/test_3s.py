"""Tion 3S over UART."""

import asyncio
import re

from aioesphomeapi import ClimateMode

from conftest import Bench, Device


async def test_state_is_published(device: Device) -> None:
    emu = device.emu3s
    emu.state.fan_speed = 3
    emu.state.target_temperature = 20

    await device.wait_for(lambda: device.value("sensor.3s_fan_speed") == 3, what="fan speed 3")
    await device.wait_for(lambda: device.state("climate.3s_climate").custom_fan_mode == "3", what="climate fan 3")
    assert device.state("climate.3s_climate").target_temperature == 20
    assert device.value("sensor.3s_outdoor_temperature") == 10
    assert device.value("text_sensor.3s_firmware_version") == "003C"
    assert device.value("select.3s_air_intake") == "outdoor"


async def test_fan_speed_from_climate(device: Device) -> None:
    emu = device.emu3s
    device.client.climate_command(device.key("climate.3s_climate"), custom_fan_mode="5")
    await device.wait_for(lambda: emu.state.fan_speed == 5, what="breezer fan speed 5")


async def test_power_off_from_climate(device: Device) -> None:
    emu = device.emu3s
    device.client.climate_command(device.key("climate.3s_climate"), mode=ClimateMode.OFF)
    await device.wait_for(lambda: not emu.state.power_state, what="breezer off")
    await device.wait_for(lambda: device.value("switch.3s_power") is False, what="power switch off")


async def test_gate_position_from_select(device: Device) -> None:
    emu = device.emu3s
    device.client.select_command(device.key("select.3s_air_intake"), "mixed")
    await device.wait_for(lambda: emu.state.gate_position == 1, what="breezer gate mixed")


async def test_antifreeze_turns_heater_on(device: Device) -> None:
    emu = device.emu3s
    emu.state.heater_state = False
    emu.state.outdoor_temperature = -5
    await device.wait_for(lambda: emu.state.heater_state, what="breezer heater on")


async def test_boost_is_started(device: Device) -> None:
    emu = device.emu3s
    device.client.switch_command(device.key("switch.3s_boost"), True)
    await device.wait_for(lambda: emu.state.fan_speed == 6, what="breezer boost speed")
    await device.wait_for(lambda: device.value("switch.3s_boost") is True, what="boost switch on")


async def test_reset_filter_requires_confirmation(device: Device) -> None:
    emu = device.emu3s
    emu.state.filter_days_left = 5

    # without confirmation button does nothing
    device.client.button_command(device.key("button.3s_reset_filter"))
    await asyncio.sleep(3)
    assert emu.state.filter_resets == 0

    device.client.switch_command(device.key("switch.3s_reset_filter_confirm"), True)
    await device.wait_for(lambda: device.value("switch.3s_reset_filter_confirm") is True, what="confirm on")
    device.client.button_command(device.key("button.3s_reset_filter"))
    await device.wait_for(lambda: emu.state.filter_resets == 1, what="breezer filter reset")


async def test_unsupported_entities_are_hidden(bench: Bench) -> None:
    device = await bench.start()
    for eid in ("switch.3s_led", "binary_sensor.3s_led", "sensor.3s_work_time", "sensor.3s_airflow_counter"):
        assert eid not in device.entities, eid
        # same entity is exposed by a model that supports it
        assert eid.replace("3s", "4s") in device.entities, eid
    log = bench.log()
    assert "Unsupported 3S Led" in log
    # unsupported entity is a model limitation, not a component failure
    assert not re.search(r"\btion\.\S+ was marked as failed", log)
