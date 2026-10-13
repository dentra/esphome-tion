"""Tion Lite over UART (text console)."""

from aioesphomeapi import ClimateFanMode, ClimateMode

from conftest import Device


async def test_state_is_published(device: Device) -> None:
    emu = device.emult
    emu.state.fan_speed = 4
    emu.state.outdoor_temperature = -3

    await device.wait_for(lambda: device.value("sensor.lt_fan_speed") == 4, what="fan speed 4")
    await device.wait_for(lambda: device.value("sensor.lt_outdoor_temperature") == -3, what="outdoor -3")
    assert device.value("text_sensor.lt_firmware_version") == "054B"
    assert device.value("binary_sensor.lt_power") is True


async def test_fan_speed_from_climate(device: Device) -> None:
    emu = device.emult
    device.client.climate_command(device.key("climate.lt_climate"), custom_fan_mode="5")
    await device.wait_for(lambda: emu.state.fan_speed == 5, what="breezer fan speed 5")
    await device.wait_for(lambda: device.state("climate.lt_climate").custom_fan_mode == "5", what="climate fan 5")


async def test_power_off_and_on(device: Device) -> None:
    emu = device.emult
    device.client.switch_command(device.key("switch.lt_power"), False)
    await device.wait_for(lambda: not emu.state.power_state, what="breezer off")
    device.client.switch_command(device.key("switch.lt_power"), True)
    await device.wait_for(lambda: emu.state.power_state, what="breezer on")
    assert emu.state.fan_speed == 2


async def test_heater_and_temperature_from_climate(device: Device) -> None:
    emu = device.emult
    device.client.climate_command(device.key("climate.lt_climate"), mode=ClimateMode.HEAT, target_temperature=22)
    await device.wait_for(lambda: emu.state.heater_state and emu.state.target_temperature == 22, what="heat 22")


async def test_kiv_from_climate_fan_off(device: Device) -> None:
    emu = device.emult
    device.client.climate_command(device.key("climate.lt_climate"), fan_mode=ClimateFanMode.OFF)
    await device.wait_for(lambda: emu.state.fan_speed == 0, what="breezer speed 0")
    assert emu.state.power_state


async def test_kiv_is_not_allowed_on_frost(device: Device) -> None:
    emu = device.emult
    emu.state.outdoor_temperature = 2
    await device.wait_for(lambda: device.value("sensor.lt_outdoor_temperature") == 2, what="outdoor 2")

    device.client.climate_command(device.key("climate.lt_climate"), fan_mode=ClimateFanMode.OFF)

    await device.wait_for(lambda: not emu.state.power_state, what="breezer off instead of kiv")
    assert emu.state.fan_speed == 2


async def test_boost_runs_and_restores_state(device: Device) -> None:
    emu = device.emult
    device.client.switch_command(device.key("switch.lt_boost"), True)
    await device.wait_for(lambda: emu.state.fan_speed == 6, what="breezer boost speed")

    # power on time drives boost timer
    emu.state.work_time += 10 * 60 + 1

    await device.wait_for(lambda: emu.state.fan_speed == 2, what="breezer speed restored")
    await device.wait_for(lambda: device.value("switch.lt_boost") is False, what="boost switch off")
