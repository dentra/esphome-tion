"""E2E fixtures: host firmware from tests/smoke with breezer emulators on pty."""

import asyncio
import os
import pty
import re
import shutil
import subprocess
import sys
import time
from collections.abc import AsyncIterator, Callable
from pathlib import Path

import pytest
from aioesphomeapi import APIClient, EntityInfo, EntityState

from emu import PtyPort, Tion3sUart, Tion4sUart, TionLtUart, TionO2Uart

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "build.tmp"
NODE_NAME = "tion-e2e"
API_PORT = 6053
# host uart port must be a two level path
PORTS = {m: f"/tmp/tion-e2e-{m}" for m in ("4s", "3s", "lt", "o2")}


@pytest.fixture(scope="session")
def firmware() -> Path:
    """Builds host firmware once per session. Set TION_E2E_NO_BUILD=1 to reuse existing build."""
    program = BUILD / "esphome" / "build" / NODE_NAME / ".pioenvs" / NODE_NAME / "program"
    if os.environ.get("TION_E2E_NO_BUILD") and program.exists():
        return program
    args = [sys.executable, "-m", "esphome", "-s", "node_name", NODE_NAME, "-s", "tion_update_interval", "2s"]
    args += ["-s", "flash_write_interval", "1s", "-s", "tion_state_timeout", "1s"]
    for m, port in PORTS.items():
        args += ["-s", f"port_{m}", port]
    # use local esphome-components when linked into components dir
    if (ROOT / "components" / "vport").exists():
        args += ["-s", "vport_source", str(ROOT / "components")]
    args += ["compile", str(ROOT / "tests" / "smoke" / "tion-smoke.yaml")]
    log = BUILD / "logs" / "e2e-build.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, ESPHOME_DATA_DIR=str(BUILD / "esphome"))
    with log.open("w") as f:
        res = subprocess.run(args, cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, check=False)
    if res.returncode != 0:
        pytest.fail(f"firmware build failed, see {log}")
    return program


def object_id(name: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def domain(obj: EntityInfo | EntityState) -> str:
    """BinarySensorInfo/BinarySensorState -> binary_sensor."""
    name = type(obj).__name__.removesuffix("Info").removesuffix("State")
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).lower()


def entity_id(info: EntityInfo) -> str:
    """Home Assistant like id: BinarySensorInfo "4S Power" -> binary_sensor.4s_power."""
    return f"{domain(info)}.{object_id(info.name)}"


class Device:
    """Running firmware connected through native API."""

    def __init__(self, client: APIClient, bench: "Bench") -> None:
        self.client = client
        self.emu4s = bench.emu4s
        self.emu3s = bench.emu3s
        self.emult = bench.emult
        self.emuo2 = bench.emuo2
        self.entities: dict[str, EntityInfo] = {}
        # entity keys are unique only within a domain
        self.states: dict[tuple[str, int], EntityState] = {}

    def entity(self, eid: str) -> EntityInfo:
        """Entity by id like "sensor.4s_fan_speed"."""
        return self.entities[eid]

    def key(self, eid: str) -> int:
        return self.entity(eid).key

    def state(self, eid: str) -> EntityState | None:
        return self.states.get((eid.split(".")[0], self.key(eid)))

    def value(self, eid: str):
        """Shortcut for state().state, None if no state yet."""
        st = self.state(eid)
        return None if st is None else st.state

    async def wait_for(self, predicate: Callable[[], bool], timeout: float = 10, what: str = "") -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            await asyncio.sleep(0.05)
        pytest.fail(f"timeout waiting for {what or predicate}")


class Bench:
    """Ports, emulators and preferences live for the whole test, firmware can be restarted."""

    def __init__(self, firmware: Path, name: str) -> None:
        self.firmware = firmware
        self.name = name
        self.prefs = BUILD / "e2e" / "prefs" / name
        shutil.rmtree(self.prefs, ignore_errors=True)
        self.prefs.mkdir(parents=True)
        self.log_dir = BUILD / "logs" / "e2e"
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.loop = asyncio.get_running_loop()
        self.ports = {m: PtyPort(p) for m, p in PORTS.items()}
        self.emu4s = Tion4sUart(self.ports["4s"])
        self.emu3s = Tion3sUart(self.ports["3s"])
        self.emult = TionLtUart(self.ports["lt"])
        self.emuo2 = TionO2Uart(self.ports["o2"])
        self.emus = [self.emu4s, self.emu3s, self.emult, self.emuo2]
        for emu in self.emus:
            emu.attach(self.loop)
        self.runs = 0
        self.device: Device | None = None
        self._proc: subprocess.Popen | None = None
        self._log_file = None

    async def start(self) -> Device:
        """Starts firmware and waits until breezer state is received."""
        assert self._proc is None, "firmware is already running"
        self.runs += 1
        suffix = "" if self.runs == 1 else f"-{self.runs}"
        self._log_file = (self.log_dir / f"{self.name}{suffix}.log").open("wb")
        env = dict(os.environ, ESPHOME_PREFDIR=str(self.prefs))
        # pty makes firmware output line buffered, so boot logs are captured even after kill
        self._out, out_slave = pty.openpty()
        self._proc = subprocess.Popen([str(self.firmware)], env=env, stdout=out_slave, stderr=out_slave)
        os.close(out_slave)
        self.loop.add_reader(self._out, self._on_output)
        client = APIClient("127.0.0.1", API_PORT, None)
        deadline = time.monotonic() + 15
        while True:
            try:
                await client.connect(login=True, log_errors=False)
                break
            except Exception:  # pylint: disable=broad-except
                if time.monotonic() > deadline or self._proc.poll() is not None:
                    raise
                await asyncio.sleep(0.2)

        dev = Device(client, self)
        self.device = dev
        entities, _ = await client.list_entities_services()
        dev.entities = {entity_id(e): e for e in entities}
        client.subscribe_states(lambda st: dev.states.__setitem__((domain(st), st.key), st))
        # commands are ignored until breezer state is received
        for m in ("4s", "3s", "lt", "o2"):
            await dev.wait_for(lambda: dev.value(f"text_sensor.{m}_firmware_version") not in (None, ""),
                               what=f"{m} state")
            await dev.wait_for(lambda: dev.state(f"climate.{m}_climate") is not None, what=f"{m} climate")
        return dev

    def _on_output(self) -> None:
        try:
            data = os.read(self._out, 4096)
        except OSError:
            data = b""
        if not data:
            self.loop.remove_reader(self._out)
            return
        self._log_file.write(re.sub(rb"\x1b\[[0-9;]*m", b"", data).replace(b"\r", b""))

    async def stop(self) -> None:
        """Kills firmware like power loss, preferences already written are kept."""
        if self.device is not None:
            await self.device.client.disconnect(force=True)
            self.device = None
        if self._proc is not None:
            self._proc.kill()
            self._proc.wait(5)
            self._proc = None
            self.loop.remove_reader(self._out)
            os.close(self._out)
        if self._log_file is not None:
            self._log_file.close()
            self._log_file = None

    async def close(self) -> None:
        await self.stop()
        for emu in self.emus:
            emu.detach(self.loop)
        for p in self.ports.values():
            p.close()


@pytest.fixture
async def bench(firmware: Path, request: pytest.FixtureRequest) -> AsyncIterator[Bench]:
    b = Bench(firmware, re.sub(r"[^a-zA-Z0-9_]+", "_", request.node.name))
    try:
        yield b
    finally:
        await b.close()


@pytest.fixture
async def device(bench: Bench) -> Device:
    return await bench.start()
