#!/usr/bin/env python3

import os
import pathlib
import sqlite3
import subprocess
import sys
import tempfile


def run(*args: str, cwd: pathlib.Path) -> None:
    result = subprocess.run(args, cwd=cwd, check=False, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr


def main() -> int:
    nandina_source = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-consumer-") as directory:
        root = pathlib.Path(directory)
        source = root / "source"
        build = root / "build"
        resources = source / "resources"
        assets = resources / "assets"
        subprojects = source / "subprojects"
        assets.mkdir(parents=True)
        subprojects.mkdir()
        os.symlink(nandina_source, subprojects / "nandina", target_is_directory=True)
        (assets / "hello.txt").write_text("hello from consumer", encoding="ascii")
        (resources / "resources.toml").write_text(
            'package_id = "org.nandina.fixture"\n\n'
            '[[roots]]\n'
            'path = "assets"\n',
            encoding="ascii",
        )
        (source / "meson.build").write_text(
            "project('nandina_consumer_fixture', 'cpp')\n"
            "nandina = subproject('nandina', default_options: "
            "['build_tests=false', 'physics2d=disabled'])\n"
            "toolchain = nandina.get_variable('nandina_resource_toolchain')\n"
            "policy = files('resources/resources.toml')\n"
            "package = custom_target(\n"
            "  'consumer-resources',\n"
            "  input: policy,\n"
            "  output: 'resources',\n"
            "  command: [toolchain['build_helper'], 'build', toolchain['nanres'], "
            "policy, '@OUTPUT@'],\n"
            "  build_by_default: true,\n"
            "  build_always_stale: true,\n"
            ")\n",
            encoding="ascii",
        )

        run("meson", "setup", str(build), "--wrap-mode=nodownload", cwd=source)
        run("meson", "compile", "-C", str(build), "consumer-resources", cwd=source)

        package = build / "resources" / "resources.db"
        assert package.is_file()
        lock = resources / "resources.lock.toml"
        assert lock.is_file()
        with sqlite3.connect(package) as database:
            keys = [row[0] for row in database.execute(
                "select resource_key from resources order by resource_key"
            )]
        assert keys == ["hello.txt"]

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
