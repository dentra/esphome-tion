"""packages/tion/auto_co2.yaml on Tion 4S over UART."""

import asyncio

import pytest

from conftest import Device

LINEAR = "Линейный"
QUADRATIC = "Квадратичный"
EXPONENTIAL = "Экспоненциальный"


async def set_number(dev: Device, eid: str, value: float) -> None:
    dev.client.number_command(dev.key(eid), value)
    await dev.wait_for(lambda: dev.value(eid) == value, what=f"{eid} = {value}")


async def configure(
    dev: Device,
    *,
    alg: str = LINEAR,
    aggressiveness: int = 3,
    setpoint: int = 700,
    min_speed: int = 1,
    max_speed: int = 5,
    ramping: bool = False,
) -> None:
    """Sets package and auto settings, min time delta is always 0 to get speed changes immediately."""
    await set_number(dev, "number.auto_min_time_delta", 0)
    await set_number(dev, "number.auto_aggressiveness", aggressiveness)
    dev.client.select_command(dev.key("select.auto_algorithm"), alg)
    await dev.wait_for(lambda: dev.value("select.auto_algorithm") == alg, what=f"algorithm {alg}")
    dev.client.switch_command(dev.key("switch.auto_use_ramping"), ramping)
    await dev.wait_for(lambda: dev.value("switch.auto_use_ramping") is ramping, what=f"ramping {ramping}")
    await set_number(dev, "number.4s_auto_setpoint", setpoint)
    # max first, min greater than max is fixed by api
    await set_number(dev, "number.4s_auto_max_fan_speed", max_speed)
    await set_number(dev, "number.4s_auto_min_fan_speed", min_speed)
    dev.client.switch_command(dev.key("switch.4s_auto"), True)
    await dev.wait_for(lambda: dev.value("switch.4s_auto") is True, what="auto on")


async def co2(dev: Device, value: int) -> None:
    dev.client.number_command(dev.key("number.fake_co2"), value)
    await dev.wait_for(lambda: dev.value("number.fake_co2") == value, what=f"co2 {value}")


async def expect_speed(dev: Device, speed: int) -> None:
    """Waits for breezer speed and for firmware to receive it, next co2 value depends on current speed."""
    emu = dev.emu4s
    await dev.wait_for(lambda: emu.state.power_state and emu.state.fan_speed == speed, what=f"breezer speed {speed}")
    await dev.wait_for(lambda: dev.value("sensor.4s_fan_speed") == speed, what=f"firmware speed {speed}")


async def expect_off(dev: Device) -> None:
    await dev.wait_for(lambda: not dev.emu4s.state.power_state, what="breezer off")
    await dev.wait_for(lambda: dev.value("binary_sensor.4s_power") is False, what="firmware power off")


async def test_linear(auto_co2_device: Device) -> None:
    dev = auto_co2_device
    await configure(dev)

    await co2(dev, 650)
    await expect_speed(dev, 1)
    # 1 + int(150 / 700 * 3 * 4)
    await co2(dev, 850)
    await expect_speed(dev, 3)
    await co2(dev, 1500)
    await expect_speed(dev, 5)


@pytest.mark.parametrize(
    ("alg", "speed"),
    [
        # 1 + int(150 / 700 * 3 * 4)
        (LINEAR, 3),
        # ratio = 150 / (1200 - 700), round(1 + 2 * 4 * ratio^2)
        (QUADRATIC, 2),
        # round(1 + 2 * 4 * (exp(ratio) - 1))
        (EXPONENTIAL, 4),
    ],
)
async def test_algorithm(auto_co2_device: Device, alg: str, speed: int) -> None:
    dev = auto_co2_device
    await configure(dev, alg=alg)

    await co2(dev, 650)
    await expect_speed(dev, 1)
    await co2(dev, 850)
    await expect_speed(dev, speed)


@pytest.mark.parametrize(
    ("aggressiveness", "speed"),
    [
        # ratio = 200 / (1200 - 700), round(1 + 6 / (6 - 1) * 4 * ratio^2)
        (1, 2),
        # round(1 + 6 / (6 - 5) * 4 * ratio^2)
        (5, 5),
    ],
)
async def test_aggressiveness(auto_co2_device: Device, aggressiveness: int, speed: int) -> None:
    dev = auto_co2_device
    await configure(dev, alg=QUADRATIC, aggressiveness=aggressiveness)

    await co2(dev, 650)
    await expect_speed(dev, 1)
    await co2(dev, 900)
    await expect_speed(dev, speed)


async def test_setpoint_above_co2_critical(auto_co2_device: Device) -> None:
    dev = auto_co2_device
    await configure(dev, alg=QUADRATIC, setpoint=1300)

    await co2(dev, 1250)
    await expect_speed(dev, 1)
    # co2 range is 100 ppm, ratio = 0.5, round(1 + 2 * 4 * ratio^2)
    await co2(dev, 1350)
    await expect_speed(dev, 3)


async def test_ramping(auto_co2_device: Device) -> None:
    dev = auto_co2_device
    await configure(dev, ramping=True)

    await co2(dev, 650)
    await expect_speed(dev, 1)
    # target speed is 5, changed by one step per co2 value
    await co2(dev, 1500)
    await expect_speed(dev, 2)
    await co2(dev, 1500)
    await expect_speed(dev, 3)


async def test_min_speed_zero_turn_on_threshold(auto_co2_device: Device) -> None:
    dev = auto_co2_device
    # turn on threshold for aggressiveness 3 is 90 ppm
    await configure(dev, min_speed=0)

    await co2(dev, 650)
    await expect_off(dev)
    # below threshold linear formula gives speed 1, but breezer must stay off
    await co2(dev, 760)
    await asyncio.sleep(1)
    assert not dev.emu4s.state.power_state
    # 1 + int(100 / 700 * 3 * 5)
    await co2(dev, 800)
    await expect_speed(dev, 3)
    # breezer keeps speed 1 until setpoint is reached
    await co2(dev, 710)
    await expect_speed(dev, 1)
    await co2(dev, 700)
    await expect_off(dev)


async def test_auto_conditions(auto_co2_device: Device) -> None:
    dev = auto_co2_device
    await dev.wait_for(lambda: dev.value("binary_sensor.auto_is_off") is True, what="auto is off")
    assert dev.value("binary_sensor.auto_is_on") is False

    # conditions must not follow boost
    dev.client.switch_command(dev.key("switch.4s_boost"), True)
    await dev.wait_for(lambda: dev.value("switch.4s_boost") is True, what="boost on")
    await asyncio.sleep(0.5)
    assert dev.value("binary_sensor.auto_is_on") is False
    assert dev.value("binary_sensor.auto_is_off") is True
    dev.client.switch_command(dev.key("switch.4s_boost"), False)
    await dev.wait_for(lambda: dev.value("switch.4s_boost") is False, what="boost off")

    dev.client.switch_command(dev.key("switch.4s_auto"), True)
    await dev.wait_for(lambda: dev.value("binary_sensor.auto_is_on") is True, what="auto is on")
    assert dev.value("binary_sensor.auto_is_off") is False
