#!/usr/bin/env python

import os
import shutil
from typing import Iterable

import click
from build import PRJ_PATH, Build, make_builds
from helpers import debug, info, trace

CONFIG_PATH = "fw"
BUILD_PATH = ".build/fota"
FOTA_JSON = "scripts/fota.json.j2"


class BuildFOTA(Build):
    def __init__(
        self,
        br_type: str,
        br_port: str,
        br_conn: str,
        is_dev,
        conf_path: str,
        fota_path: str,
    ) -> None:
        super().__init__(br_type, br_port, br_conn, is_dev, conf_path)
        self.fota_path = fota_path
        assert self.fota_path

    @property
    def manifest_file(self):
        return os.path.join(self.fota_path, f"{self.fw_name}.json")

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

    @property
    def fota_ota_md5(self):
        return self.md5(self.fota_ota_bin)

    @property
    def fota_ota_bin(self):
        return os.path.join(self.fota_path, f"{self.fw_name}.ota.bin")

    @property
    def fota_factory_bin(self):
        return os.path.join(self.fota_path, f"{self.fw_name}.factory.bin")


@click.command()
@click.argument("version")
@click.option("--verbose", "-v", count=True)
@click.option("--no-compile", "-C", is_flag=True)
@click.option("--clean", "-c", is_flag=True)
@click.option("--dev", "-d", is_flag=True)
@click.option("--factory", "-f", is_flag=True)
@click.option(
    "--types",
    "-t",
    default="4s-uart-mqtt",
    show_default=True,
)
def main(
    version: str,
    no_compile: bool,
    clean: bool,
    dev: bool,
    factory: bool,
    verbose: int,
    types: str,
):
    builds = make_builds(
        version=version,
        compile=not no_compile,
        clean=clean,
        dev=dev,
        verbose=verbose,
        config_path=CONFIG_PATH,
        conf_types=[tuple(br.split("-")) for br in types.split(" ")],
        build_type=BuildFOTA,
        build_sub_folder="fota",
        fota_path=os.path.join(PRJ_PATH, BUILD_PATH, version),
    )

    make_manifest(
        prj_path=PRJ_PATH,
        version_tion=version,
        builds=builds,
        do_factory=factory,
    )


def make_manifest(
    prj_path: str, version_tion: str, builds: Iterable[BuildFOTA], do_factory: bool
):
    print()
    info("*" * 80)
    info("Generating FOTA assets...")

    for build in builds:
        debug(build)

        trace(f"fota_path  : {build.fota_path}")
        trace(f"manifest   : {build.manifest_file}")
        trace(f"ota_bin    : {build.fota_ota_bin}")
        if do_factory:
            trace(f"factory_bin: {build.fota_factory_bin}")

        info("Copying binaries...")
        os.makedirs(build.fota_path, exist_ok=True)
        shutil.copy(build.ota_bin, build.fota_ota_bin)
        if do_factory:
            shutil.copy(build.factory_bin, build.fota_factory_bin)

        info(f"Generating manifest {build.manifest_file}")
        with open(build.manifest_file, mode="w", encoding="utf-8") as out:
            import jinja2

            j2env = jinja2.Environment(loader=jinja2.FileSystemLoader(prj_path))
            j2tpl = j2env.get_template(FOTA_JSON)
            out.write(
                j2tpl.render(
                    version_tion=version_tion,
                    build=build,
                    has_factory=do_factory,
                )
            )


if __name__ == "__main__":
    # pylint: disable=no-value-for-parameter
    main()
