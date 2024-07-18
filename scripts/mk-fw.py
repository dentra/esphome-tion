#!/usr/bin/env python

import os
import pathlib
import shutil
import sys

import click

from esphome.components.esp32.boards import VARIANT_ESP32S3
from esphome.const import CONF_VARIANT
from esphome.core import CORE

CONFIG_PATH = "fw"
BUILD_PATH = ".build/fota"
FOTA_JSON = "scripts/fota.json.j2"

DEFAULT_TYPES = [
    "lt-ble",
    "lt-uart",
    "4s-ble",
    "4s-uart",
    "3s-ble",
    "3s-uart",
    "o2-uart",
]

DEFAULT_CONNS = [
    "mqtt",
    "api",
]

CHIP_FAMILY = {
    VARIANT_ESP32S3: "ESP32-S3",
}

g_verbose = 0


@click.command()
@click.argument("version")
@click.option("--compile/--no-compile", default=True)
@click.option("--clean/--no-clean", default=False)
@click.option("--dev/--no-dev", default=False)
@click.option("--factory/--no-factory", default=False)
@click.option("-v", "--verbose", count=True)
def main(
    version: str, compile: bool, clean: bool, dev: bool, factory: bool, verbose: int
):
    global g_verbose
    g_verbose = verbose
    if version in ["auto", "detect"]:
        import subprocess

        proc = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            stdout=subprocess.PIPE,
            encoding="utf-8",
            check=False,
        )
        if proc.returncode != 0:
            error("Failed detect version", proc.returncode)
        version = proc.stdout.strip()

    prj_path = "."

    info(f"Build version: {version}")
    debug(f"  compile: {compile}")
    debug(f"    clean: {clean}")
    debug(f"      dev: {dev}")
    debug(f" prj_path: {prj_path}")

    types = DEFAULT_TYPES
    conns = DEFAULT_CONNS

    builds: list[Build] = []

    for type in types:
        type = type.split("-")
        br_type = type[0]
        br_port = type[1]
        for br_conn in conns:
            if build := process(
                prj_path=prj_path,
                br_type=br_type,
                br_port=br_port,
                br_conn=br_conn,
                is_dev=dev,
                do_compile=compile,
                do_clean=clean,
            ):
                debug(build)
                builds.append(build)

    make_manifest(
        prj_path=prj_path,
        version_tion=version,
        builds=builds,
        do_factory=factory,
    )


def trace(msg):
    global g_verbose
    if g_verbose > 1:
        click.secho(msg, fg="black")


def debug(msg):
    global g_verbose
    if g_verbose > 0:
        click.secho(msg, fg="black")


def info(msg):
    click.secho(msg, fg="green")


def warn(msg):
    click.secho(msg, fg="yellow")


def error(msg, exit_code=1):
    click.secho(msg, fg="red")
    sys.exit(exit_code)


def load_config(filename: str, fw_name: str):
    from esphome.config import read_config

    CORE.reset()
    CORE.config_path = filename
    CORE.config = read_config({"fw_name": fw_name})
    CORE.build_path = CORE.relative_internal_path(f"build/fota/{fw_name}")
    return CORE.config


def clean_build_files():
    from esphome.writer import clean_build

    clean_build()


def compile_program(config, verbose=False):
    import functools

    import esphome.codegen as cg
    from esphome import platformio_api, writer, yaml_util
    from esphome.config import iter_component_configs
    from esphome.core import coroutine
    from esphome.helpers import indent

    def wrap_to_code(name, comp):
        coro = coroutine(comp.to_code)

        @functools.wraps(comp.to_code)
        async def wrapped(conf):
            cg.add(cg.LineComment(f"{name}:"))
            if comp.config_schema is not None:
                conf_str = yaml_util.dump(conf)
                conf_str = conf_str.replace("//", "")
                # remove tailing \ to avoid multi-line comment warning
                conf_str = conf_str.replace("\\\n", "\n")
                cg.add(cg.LineComment(indent(conf_str)))
            await coro(conf)

        if hasattr(coro, "priority"):
            wrapped.priority = coro.priority
        return wrapped

    def generate_cpp_contents(config):
        info("Generating C++ source...")

        for name, component, conf in iter_component_configs(config):
            if component.to_code is not None:
                coro = wrap_to_code(name, component)
                CORE.add_job(coro, conf)

        CORE.flush_tasks()

    def write_cpp_file():
        writer.write_platformio_project()

        code_s = indent(CORE.cpp_main_section)
        writer.write_cpp(code_s)
        return 0

    def write_cpp(config):
        generate_cpp_contents(config)
        return write_cpp_file()

    if rc := write_cpp(config) != 0:
        error("Failed generate cpp", rc)
    info("Successfully generated source code.")

    if rc := platformio_api.run_compile(config, verbose) != 0:
        error("Failed compile", rc)
    info("Successfully compiled program.")


class Build:
    def __init__(self, fw_name: str) -> None:
        self.fw_name = fw_name
        self.chip_family = CHIP_FAMILY[CORE.config[CORE.target_platform][CONF_VARIANT]]
        self.build_path = CORE.relative_pioenvs_path(CORE.name)

    @staticmethod
    def md5(fname: str):
        import hashlib

        hash_md5 = hashlib.md5()
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        digest = hash_md5.hexdigest()
        trace(f"md5: {digest} {fname}")
        return digest

    def __str__(self) -> str:
        return f"Build[fw_name={self.fw_name}, chip_family={self.chip_family}, build_path={self.build_path}]"


def process(
    prj_path: str,
    br_type: str,
    br_port: str,
    br_conn: str,
    is_dev: bool,
    do_compile: bool,
    do_clean: bool,
) -> list[Build] | None:
    print()
    info("*" * 80)
    info(f"Processing {br_type} {br_port} {br_conn}")

    fw_name = f"tion-{br_type}-{br_port}-{br_conn}{'-dev' if is_dev else ''}"

    config_file = f"{prj_path}/{CONFIG_PATH}/{fw_name}.yaml"

    if not pathlib.Path(config_file).is_file():
        warn("Configuration does not exists")
        return None

    config = load_config(config_file, fw_name)

    trace(f"config_file: {config_file}")
    trace(f"fw_name    : {fw_name}")
    trace(f"node_name  : {CORE.name}")

    if do_clean:
        info("Clean build files")
        clean_build_files()

    if do_compile:
        info(f"Compiling {config_file}")
        compile_program(config)
    else:
        warn(f"Skip compilation {config_file}")

    return Build(fw_name)


def make_manifest(
    prj_path: str, version_tion: str, builds: list[Build], do_factory: bool
):
    print()
    info("*" * 80)

    fota_path = f"{prj_path}/{BUILD_PATH}/{version_tion}"

    for build in builds:
        debug(build)
        manifest = f"{fota_path}/{build.fw_name}.json"

        ota_bin = f"{fota_path}/{build.fw_name}.ota.bin"
        factory_bin = f"{fota_path}/{build.fw_name}.factory.bin"

        trace(f"fota_path  : {fota_path}")
        trace(f"manifest   : {manifest}")
        trace(f"ota_bin    : {ota_bin}")
        if do_factory:
            trace(f"factory_bin: {factory_bin}")

        info("Copying binaries")
        os.makedirs(fota_path, exist_ok=True)
        for src, dst in [
            ("firmware.ota.bin", ota_bin),
            ("firmware.factory.bin", factory_bin) if do_factory else (None, None),
        ]:
            if src and dst:
                shutil.copy(f"{build.build_path}/{src}", dst)

        info(f"Generating manifest {manifest}")
        with open(manifest, mode="w", encoding="utf-8") as out:
            import jinja2

            j2env = jinja2.Environment(loader=jinja2.FileSystemLoader(prj_path))
            j2tpl = j2env.get_template(FOTA_JSON)
            out.write(
                j2tpl.render(
                    version_tion=version_tion,
                    fw_name=build.fw_name,
                    ota_md5=build.md5(ota_bin),
                    factory_md5=build.md5(factory_bin) if do_factory else None,
                    chip_family=build.chip_family,
                )
            )


if __name__ == "__main__":
    main()
