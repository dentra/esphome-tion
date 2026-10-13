"""Configuration level tests: validated config produced by `esphome config`."""

import os
import subprocess
import sys
from functools import cache
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "build.tmp"
GEN_CONFIGS = sorted((ROOT / "tests" / "gen").glob("tion-*.yaml"))


class _Loader(yaml.SafeLoader):
    """Keeps esphome tags like !lambda or !secret as plain values."""


_Loader.add_multi_constructor("!", lambda loader, suffix, node: loader.construct_scalar(node)
                              if isinstance(node, yaml.ScalarNode) else None)


@cache
def validated(config: Path) -> dict:
    """Returns config validated by esphome."""
    env = dict(os.environ, ESPHOME_DATA_DIR=str(BUILD / "esphome"), ESPHOME_NOGITIGNORE="1")
    res = subprocess.run([sys.executable, "-m", "esphome", "config", str(config)], cwd=ROOT, env=env,
                         capture_output=True, text=True, check=False)
    assert res.returncode == 0, res.stdout + res.stderr
    return yaml.load(res.stdout, Loader=_Loader)


def includes_spruthub(config: Path) -> bool:
    return any(line.strip().startswith("spruthub:") for line in config.read_text().splitlines())


@pytest.mark.parametrize("config", GEN_CONFIGS, ids=lambda p: p.stem)
def test_main_climate_name(config: Path) -> None:
    """SprutHub expects main climate without own name, so topics are <prefix>/climate/<device name>/...
    Otherwise it is named Climate."""
    climate = next((c for c in validated(config).get("climate", []) if c.get("id") == "tion_climate"), None)
    if climate is None:
        pytest.skip("no main climate")
    # entity without name is internal, name must be present
    assert "name" in climate
    assert not climate.get("internal", False)
    if includes_spruthub(config):
        assert climate["name"] == ""
    else:
        assert climate["name"] == "Climate"


def test_spruthub_is_covered() -> None:
    """At least one config with spruthub package and one without."""
    with_spruthub = [c for c in GEN_CONFIGS if includes_spruthub(c)]
    assert with_spruthub and len(with_spruthub) < len(GEN_CONFIGS)
