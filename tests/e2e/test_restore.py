"""restore_state: breezer state is restored after power loss (enabled for 4S in tests/smoke)."""

import asyncio

from aioesphomeapi import ClimateMode

from conftest import Bench
from emu import Tion4sState, Tion4sUart

# flash_write_interval is 1s in e2e build
PREFS_SYNC = 2.5


def power_loss(emu: Tion4sUart) -> None:
    """Breezer after power loss: turned off with default settings, counters are kept."""
    emu.state = Tion4sState(
        power_state=False,
        heater_state=False,
        fan_speed=1,
        target_temperature=10,
        work_time=emu.state.work_time,
    )
    emu.frames.clear()


async def test_state_is_restored_after_power_loss(bench: Bench) -> None:
    dev = await bench.start()
    emu = bench.emu4s
    dev.client.climate_command(dev.key("climate.4s_climate"), mode=ClimateMode.HEAT, target_temperature=20,
                               custom_fan_mode="4")
    await dev.wait_for(lambda: emu.state.fan_speed == 4 and emu.state.heater_state, what="breezer applied state")
    await asyncio.sleep(PREFS_SYNC)
    await bench.stop()

    power_loss(emu)
    await bench.start()

    await bench.device.wait_for(lambda: emu.state.power_state, what="breezer power restored")
    assert emu.state.fan_speed == 4
    assert emu.state.heater_state
    assert emu.state.target_temperature == 20


async def test_power_off_is_restored(bench: Bench) -> None:
    dev = await bench.start()
    emu = bench.emu4s
    dev.client.switch_command(dev.key("switch.4s_power"), False)
    await dev.wait_for(lambda: not emu.state.power_state, what="breezer off")
    await asyncio.sleep(PREFS_SYNC)
    await bench.stop()

    # breezer is turned on after power loss
    power_loss(emu)
    emu.state.power_state = True
    await bench.start()

    await bench.device.wait_for(lambda: not emu.state.power_state, what="breezer turned off again")


async def test_nothing_is_written_without_saved_state(bench: Bench) -> None:
    await bench.start()
    emu = bench.emu4s
    # several polls
    await asyncio.sleep(5)
    assert not emu.received(Tion4sUart.STATE_SET)


async def test_state_is_restored_once(bench: Bench) -> None:
    dev = await bench.start()
    emu = bench.emu4s
    dev.client.climate_command(dev.key("climate.4s_climate"), custom_fan_mode="4")
    await dev.wait_for(lambda: emu.state.fan_speed == 4, what="breezer speed 4")
    await asyncio.sleep(PREFS_SYNC)
    await bench.stop()

    power_loss(emu)
    await bench.start()
    await bench.device.wait_for(lambda: emu.state.fan_speed == 4, what="breezer speed restored")
    sets = len(emu.received(Tion4sUart.STATE_SET))

    # user changes speed with remote, firmware must not restore it again
    emu.state.fan_speed = 2
    await asyncio.sleep(5)
    assert emu.state.fan_speed == 2
    assert len(emu.received(Tion4sUart.STATE_SET)) == sets


async def test_boost_is_not_saved(bench: Bench) -> None:
    dev = await bench.start()
    emu = bench.emu4s
    # initial state (speed 2) is saved on first state
    await asyncio.sleep(PREFS_SYNC)
    dev.client.switch_command(dev.key("switch.4s_boost"), True)
    await dev.wait_for(lambda: emu.state.fan_speed == 6, what="boost speed")
    await asyncio.sleep(PREFS_SYNC)
    await bench.stop()

    power_loss(emu)
    await bench.start()

    await bench.device.wait_for(lambda: emu.state.power_state, what="breezer power restored")
    assert emu.state.fan_speed == 2
