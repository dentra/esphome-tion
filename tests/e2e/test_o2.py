"""Tion O2 over UART."""

from conftest import Device


async def test_state_is_published(device: Device) -> None:
    emu = device.emuo2
    emu.state.fan_speed = 3
    emu.state.productivity = 90

    await device.wait_for(lambda: device.value("sensor.o2_fan_speed") == 3, what="fan speed 3")
    await device.wait_for(lambda: device.value("sensor.o2_productivity") == 90, what="productivity 90")
    assert device.value("text_sensor.o2_firmware_version") == "130E"


async def test_fan_speed_from_climate(device: Device) -> None:
    emu = device.emuo2
    climate = device.entity("climate.o2_climate")
    assert climate.supported_custom_fan_modes == ["1", "2", "3", "4"]

    device.client.climate_command(climate.key, custom_fan_mode="4")
    await device.wait_for(lambda: emu.state.fan_speed == 4, what="breezer fan speed 4")


async def test_power_off_from_switch(device: Device) -> None:
    emu = device.emuo2
    device.client.switch_command(device.key("switch.o2_power"), False)
    await device.wait_for(lambda: not emu.state.power_state, what="breezer off")


async def test_antifreeze_turns_heater_on(device: Device) -> None:
    emu = device.emuo2
    emu.state.heater_state = False
    emu.state.outdoor_temperature = -5
    await device.wait_for(lambda: emu.state.heater_state, what="breezer heater on")


async def test_boost_runs_and_restores_state(device: Device) -> None:
    emu = device.emuo2
    device.client.switch_command(device.key("switch.o2_boost"), True)
    await device.wait_for(lambda: emu.state.fan_speed == 4, what="breezer boost speed")

    emu.state.work_time += 10 * 60 + 1

    await device.wait_for(lambda: emu.state.fan_speed == 2, what="breezer speed restored")
    await device.wait_for(lambda: device.value("switch.o2_boost") is False, what="boost switch off")

