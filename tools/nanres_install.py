#!/usr/bin/env python3

import os
import pathlib
import shutil
import sys


def main() -> int:
    source = pathlib.Path(sys.argv[1])
    relative_destination = pathlib.Path(sys.argv[2])
    prefix = pathlib.Path(os.environ["MESON_INSTALL_DESTDIR_PREFIX"])
    destination = prefix / relative_destination
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
