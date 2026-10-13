"""Tion 4S over UART."""

from aioesphomeapi import ClimateMode

from conftest import Device


async def test_state_is_published(device: Device) -> None:
    emu = device.emu4s
    emu.state.fan_speed = 3
    emu.state.target_temperature = 18
    emu.state.outdoor_temperature = -7

    await device.wait_for(lambda: device.value("sensor.4s_fan_speed") == 3, what="fan speed 3")
    await device.wait_for(lambda: device.state("climate.4s_climate").custom_fan_mode == "3", what="climate fan mode 3")
    climate = device.state("climate.4s_climate")
    assert climate.target_temperature == 18
    assert device.value("sensor.4s_outdoor_temperature") == -7
    assert device.value("text_sensor.4s_firmware_version") == "0230"
    assert device.value("binary_sensor.4s_state") is False


async def test_fan_speed_from_climate(device: Device) -> None:
    emu = device.emu4s
    await device.wait_for(lambda: device.state("climate.4s_climate") is not None, what="climate state")

    device.client.climate_command(device.key("climate.4s_climate"), custom_fan_mode="5")

    await device.wait_for(lambda: emu.state.fan_speed == 5, what="breezer fan speed 5")
    await device.wait_for(lambda: device.state("climate.4s_climate").custom_fan_mode == "5", what="climate fan mode 5")


async def test_power_off_from_switch(device: Device) -> None:
    emu = device.emu4s
    await device.wait_for(lambda: device.value("switch.4s_power") is True, what="power on")

    device.client.switch_command(device.key("switch.4s_power"), False)

    await device.wait_for(lambda: not emu.state.power_state, what="breezer power off")
    await device.wait_for(lambda: device.state("climate.4s_climate").mode == ClimateMode.OFF, what="climate off")


async def test_boost_runs_and_restores_state(device: Device) -> None:
    emu = device.emu4s
    await device.wait_for(lambda: device.value("switch.4s_boost") is False, what="boost off")

    device.client.switch_command(device.key("switch.4s_boost"), True)

    await device.wait_for(lambda: emu.state.fan_speed == 6, what="breezer boost speed")
    await device.wait_for(lambda: device.value("switch.4s_boost") is True, what="boost switch on")

    # boost time is 10 min by default, breezer work time drives boost timer
    emu.state.work_time += 10 * 60 + 1

    await device.wait_for(lambda: emu.state.fan_speed == 2, what="breezer speed restored")
    await device.wait_for(lambda: device.value("switch.4s_boost") is False, what="boost switch off")


async def test_state_problem_when_breezer_is_silent(device: Device) -> None:
    emu = device.emu4s
    await device.wait_for(lambda: device.value("binary_sensor.4s_state") is False, what="state ok")

    emu.online = False

    await device.wait_for(lambda: device.value("binary_sensor.4s_state") is True, what="state problem")


async def test_preset_from_select(device: Device) -> None:
    emu = device.emu4s
    options = device.entity("select.4s_presets").options
    # boost preset has a reserved slot and goes first
    assert options == ["None", "boost", "home", "away", "custom"]

    device.client.select_command(device.key("select.4s_presets"), "home")

    await device.wait_for(lambda: emu.state.fan_speed == 2 and emu.state.target_temperature == 16, what="home preset")
    await device.wait_for(lambda: device.value("select.4s_presets") == "home", what="home preset is active")
