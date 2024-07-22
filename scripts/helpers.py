import sys

import click

GITHUB_RELEASES_ULR = "https://api.github.com/repos/dentra/esphome-tion/releases"

_g_verbose = 0


def detect_github_beta():
    import json
    import urllib.request

    with urllib.request.urlopen(f"{GITHUB_RELEASES_ULR}") as url:
        data = json.load(url)

    for rel in data:
        print(
            "name",
            rel["name"],
            "tag_name",
            rel["tag_name"],
            "draft",
            rel["draft"],
            "prerelease",
            rel["prerelease"],
        )

    data = list(filter(lambda x: x["prerelease"], data))
    if not data:
        error("no one draft release found", 1)
    if len(data) > 1:
        error("more than one draft release found", 1)
    data = data[0]

    info(data["tag_name"])
    error("detect_github_beta not implementd", 1)


def detect_github_latest():
    import json
    import urllib.request

    with urllib.request.urlopen(f"{GITHUB_RELEASES_ULR}/latest") as url:
        data = json.load(url)
    return data["name"]


def detect_git_branch():
    import subprocess

    proc = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        check=False,
    )
    if proc.returncode != 0:
        error("Failed detect version", proc.returncode)
    return proc.stdout.strip()


def set_verbose(verbose: int):
    global _g_verbose
    _g_verbose = verbose


def get_verbose() -> int:
    global _g_verbose
    return _g_verbose


def trace(msg):
    if get_verbose() > 1:
        click.secho(msg, fg="black")


def debug(msg):
    if get_verbose() > 0:
        click.secho(msg, fg="black")


def info(msg):
    click.secho(msg, fg="green")


def warn(msg):
    click.secho(msg, fg="yellow")


def error(msg, exit_code=1):
    click.secho(msg, fg="red")
    sys.exit(exit_code)
