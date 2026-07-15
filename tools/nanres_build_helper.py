#!/usr/bin/env python3

import os
import pathlib
import subprocess
import sys


def run(
    nanres: str,
    policy: pathlib.Path,
    command: str,
    *args: str,
    capture_failure: bool = False,
) -> bool:
    result = subprocess.run(
        [nanres, command, *args],
        cwd=policy.parent,
        check=False,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        if capture_failure:
            return False
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    return True


def main() -> int:
    mode, nanres, policy_text, output_text = sys.argv[1:5]
    policy = pathlib.Path(policy_text).resolve()
    output = pathlib.Path(output_text).resolve()
    if policy.name != "resources.toml":
        raise SystemExit("nanres helper requires a resources.toml policy")

    if mode != "build":
        raise SystemExit(f"unknown nanres helper mode: {mode}")

    if not run(nanres, policy, "validate", capture_failure=True):
        run(nanres, policy, "scan")
        run(nanres, policy, "validate")
    relative_output = os.path.relpath(output, policy.parent)
    run(nanres, policy, "pack", "--output", relative_output)
    if not (output / "resources.db").is_file():
        raise SystemExit("nanres pack did not produce resources.db")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
