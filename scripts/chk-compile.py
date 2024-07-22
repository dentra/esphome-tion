#!/usr/bin/env python


import click
from build import Build, make_builds

CONFIG_PATH = "./configs"


@click.command()
@click.argument("version")
@click.option("-v", "--verbose", count=True)
@click.option("--clean", "-c", is_flag=True)
@click.option("--dev", "-d", is_flag=True)
@click.option(
    "--types",
    "-t",
    default="lt-ble 4s-ble 4s-uart 3s-ble 3s-uart o2-uart",
    show_default=True,
)
def main(version: str, clean: bool, dev: bool, verbose: int, types: str):
    make_builds(
        version=version,
        compile=True,
        clean=clean,
        dev=dev,
        verbose=verbose,
        config_path=CONFIG_PATH,
        conf_types=[tuple(br.split("-")) + ("",) for br in types.split(" ")],
        build_type=Build,
        build_sub_folder="test",
    )


if __name__ == "__main__":
    # pylint: disable=no-value-for-parameter
    main()
