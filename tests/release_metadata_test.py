#!/usr/bin/env python3

import pathlib
import re
import sys
import tomllib


SEMVER = re.compile(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")


def main() -> int:
    root = pathlib.Path(sys.argv[1]).resolve()
    metadata = tomllib.loads((root / "release-metadata.toml").read_text(encoding="utf-8"))
    meson = (root / "meson.build").read_text(encoding="utf-8")
    bundle = (root / "tools/sdk_bundle.py").read_text(encoding="utf-8")
    changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
    acceptance = (root / "docs/development/1.0_ACCEPTANCE.md").read_text(encoding="utf-8")

    assert metadata["schema"] == 1
    assert metadata["name"] == "NandinaUI"
    assert SEMVER.fullmatch(metadata["sdk_version"])
    assert SEMVER.fullmatch(metadata["minimum_cli"])
    assert metadata["release_channel"] == "pre-1.0"
    assert metadata["support_profile"] == "linux-desktop-source"
    assert metadata["revision"] == "unreleased"
    assert metadata["license_status"] == "pending"
    assert metadata["known_limitations"]

    project_version = re.search(r"version:\s*'([^']+)'", meson)
    assert project_version and project_version.group(1) == metadata["sdk_version"]
    assert 'parser.add_argument("--minimum-cli")' in bundle
    assert 'metadata.get("minimum_cli"' in bundle
    assert 'metadata.get("sdk_version"' in bundle

    assert changelog.startswith("# Changelog\n\n## Unreleased (pre-1.0)")
    assert "installation into system" in changelog
    assert "Windows/macOS" in changelog
    assert "C6.1" in acceptance
    assert "license status" in acceptance
    assert not any((root / name).is_file() for name in ("LICENSE", "LICENSE.txt", "COPYING"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
