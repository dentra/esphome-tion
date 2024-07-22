import os
import pathlib
from typing import Iterable

from helpers import (
    debug,
    detect_git_branch,
    detect_github_beta,
    detect_github_latest,
    error,
    get_verbose,
    info,
    set_verbose,
    trace,
    warn,
)

from esphome.const import CONF_VARIANT
from esphome.core import CORE

PRJ_PATH = "."


class Build:
    def __init__(
        self,
        br_type: str,
        br_port: str,
        br_conn: str,
        is_dev,
        conf_path: str,
    ) -> None:
        self.br_type = br_type
        self.br_port = br_port
        self.br_conn = br_conn
        self.is_dev = is_dev
        # set by configure
        self.variant = None
        # set by configure
        self.build_path = None
        self.conf_path = conf_path

    @property
    def fw_name(self):
        fw_name = f"tion-{self.br_type}-{self.br_port}"
        if self.br_conn:
            fw_name += f"-{self.br_conn}"
        if self.is_dev:
            fw_name += "-dev"
        return fw_name

    @property
    def config_file(self):
        return os.path.join(self.conf_path, f"{self.fw_name}.yaml")

    def configure(self, build_sub_folder: str | None):
        from esphome.config import read_config

        CORE.reset()
        CORE.config_path = self.config_file
        CORE.config = read_config({"fw_name": self.fw_name})

        if build_sub_folder:
            # change build_path after configure
            CORE.build_path = CORE.relative_internal_path(
                "build", build_sub_folder, self.fw_name
            )

        trace(f"config_path  : {CORE.config_path}")
        trace(f"build_path   : {
            os.path.join(".", os.path.relpath(CORE.build_path))}")
        trace(f"pioenvs_path : {
            os.path.join(".", os.path.relpath(CORE.relative_pioenvs_path(CORE.name)))}")
        trace(f"node_name    : {CORE.name}")
        trace(f"friendly_name: {CORE.friendly_name}")

        if not CORE.config:
            error(f"Failed loading config {self.config_file}", 1)

        self.build_path = CORE.relative_pioenvs_path(CORE.name)
        self.platform = CORE.target_platform
        if CORE.is_esp32:
            self.variant = CORE.config[CORE.target_platform][CONF_VARIANT]
        elif CORE.is_esp8266:
            self.variant = "ESP8266"

        return CORE.config

    def clean(self):
        from esphome.writer import clean_build

        clean_build()

    def compile(self, config):
        _compile_program(config, verbose=get_verbose() > 2)

    def __str__(self) -> str:
        return f"Build[fw_name={self.fw_name}, variant={self.variant}, build_path={self.build_path}]"

    @property
    def ota_bin(self):
        return os.path.join(self.build_path, "firmware.ota.bin")

    @property
    def factory_bin(self):
        return os.path.join(self.build_path, "firmware.factory.bin")


def _compile_program(config, verbose=False):
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


def build_config(
    build: Build, do_compile: bool, do_clean: bool, build_sub_folder: str | None = None
) -> bool:
    print()
    info("*" * 80)
    info(f"Processing {build.fw_name} to {build_sub_folder}")

    if not pathlib.Path(build.config_file).is_file():
        warn("Configuration does not exists")
        return False

    config = build.configure(build_sub_folder)

    if do_clean:
        info("Clean build files")
        build.clean()

    if do_compile:
        info(f"Compiling {build.config_file}")
        build.compile(config)
    else:
        warn(f"Skip compilation {build.config_file}")

    return True


def make_builds(
    version: str,
    compile: bool,
    clean: bool,
    dev: bool,
    verbose: int,
    config_path: str,
    conf_types: Iterable[tuple[str, str, str]],
    build_type=Build,
    build_sub_folder: str | None = None,
    **build_args,
) -> Iterable[Build]:
    set_verbose(verbose)

    if version in ["auto", "detect"]:
        version = detect_git_branch()

    if version == "latest":
        version = detect_github_latest()

    if version == "beta":
        version = detect_github_beta()

    info(f"Build version: {version}")
    debug(f"   compile: {compile}")
    debug(f"     clean: {clean}")
    debug(f"       dev: {dev}")
    debug(f"  prj_path: {PRJ_PATH}")
    debug(f"build_type: {build_type}")
    debug(f"build_args: {build_args}")
    debug(f"build_subf: {build_sub_folder}")

    builds: list[build_type] = []
    for br_type, br_port, br_conn in conf_types:
        build = build_type(
            br_type=br_type,
            br_port=br_port,
            br_conn=br_conn,
            is_dev=dev,
            conf_path=f"{PRJ_PATH}/{config_path}",
            **build_args,
        )
        if build_config(
            build=build,
            do_compile=compile,
            do_clean=clean,
            build_sub_folder=build_sub_folder,
        ):
            builds.append(build)

    return builds
