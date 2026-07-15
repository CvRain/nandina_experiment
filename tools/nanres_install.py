#!/usr/bin/env python3

import os
import pathlib
import shutil
import sys
import tomllib


def main() -> int:
    source = pathlib.Path(sys.argv[1])
    policy = pathlib.Path(sys.argv[2])
    with policy.open("rb") as stream:
        package_id = tomllib.load(stream).get("package_id")
    if not isinstance(package_id, str) or not package_id:
        raise SystemExit("nanres install requires package_id in resources.toml")
    datadir = pathlib.Path(sys.argv[3])
    prefix = pathlib.Path(os.environ["MESON_INSTALL_DESTDIR_PREFIX"])
    destination = prefix / datadir / package_id
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
