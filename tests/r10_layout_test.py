#!/usr/bin/env python3

import pathlib
import shutil
import subprocess
import sys
import tempfile


def probe(executable: str, app_id: str, app_path: pathlib.Path, expected: str) -> None:
    result = subprocess.run(
        [executable, app_id, str(app_path), expected],
        check=False,
        text=True,
        capture_output=True,
    )
    assert result.returncode == 0, result.stderr


def main() -> int:
    probe_executable = str(pathlib.Path(sys.argv[1]).resolve())
    package = pathlib.Path(sys.argv[2]).resolve()
    install_script = pathlib.Path(sys.argv[3]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-r10-") as directory:
        root = pathlib.Path(directory)

        portable_bin = root / "portable/bin"
        portable_resources = portable_bin / "resources"
        portable_bin.mkdir(parents=True)
        shutil.copytree(package, portable_resources)
        probe(probe_executable, "org.nandina.todo", portable_bin / "todo", "package")

        bare_bin = root / "bare/bin"
        bare_bin.mkdir(parents=True)
        probe(probe_executable, "org.nandina.bare", bare_bin / "app", "builtin")

        for prefix in (root / "system/usr", root / "user/home/test/.local"):
            environment = {"MESON_INSTALL_DESTDIR_PREFIX": str(prefix)}
            result = subprocess.run(
                [sys.executable, str(install_script), str(package), "share/org.nandina.todo"],
                check=False,
                text=True,
                capture_output=True,
                env=environment,
            )
            assert result.returncode == 0, result.stderr
            installed = prefix / "share/org.nandina.todo/resources.db"
            assert installed.is_file()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
