#!/usr/bin/env python3

import os
import pathlib
import subprocess
import sys
import tempfile


def install(script: pathlib.Path, source: pathlib.Path, prefix: pathlib.Path, datadir: str) -> pathlib.Path:
    environment = os.environ.copy()
    environment["MESON_INSTALL_DESTDIR_PREFIX"] = str(prefix)
    result = subprocess.run(
        [sys.executable, str(script), str(source), f"{datadir}/org.nandina.test"],
        check=False,
        text=True,
        capture_output=True,
        env=environment,
    )
    assert result.returncode == 0, result.stderr
    return prefix / datadir / "org.nandina.test"


def main() -> int:
    script = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="nandina-install-") as directory:
        root = pathlib.Path(directory)
        source = root / "package"
        source.mkdir()
        (source / "resources.db").write_bytes(b"database")
        (source / "external").mkdir()
        (source / "external/resource-id").write_bytes(b"sidecar")

        system = install(script, source, root / "stage/usr", "share")
        assert (system / "resources.db").read_bytes() == b"database"
        assert (system / "external/resource-id").read_bytes() == b"sidecar"

        user = install(script, source, root / "stage/home/test/.local", "share")
        assert (user / "resources.db").read_bytes() == b"database"
        assert (user / "external/resource-id").read_bytes() == b"sidecar"

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
