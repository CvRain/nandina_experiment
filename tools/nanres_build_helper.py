#!/usr/bin/env python3

import os
import pathlib
import subprocess
import sys


def run(nanres: str, policy: pathlib.Path, command: str, *args: str) -> None:
    result = subprocess.run(
        [nanres, command, *args],
        cwd=policy.parent,
        check=False,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)


def main() -> int:
    mode, nanres, policy_text, lock_text, output_text = sys.argv[1:6]
    policy = pathlib.Path(policy_text).resolve()
    lock = pathlib.Path(lock_text).resolve()
    output = pathlib.Path(output_text).resolve()
    if policy.name != "resources.toml" or lock.name != "resources.lock.toml":
        raise SystemExit("nanres helper requires canonical manifest filenames")

    run(nanres, policy, "validate")
    if mode == "validate":
        output.write_text("validated\n", encoding="ascii")
        return 0
    if mode != "pack":
        raise SystemExit(f"unknown nanres helper mode: {mode}")

    relative_output = os.path.relpath(output, policy.parent)
    run(nanres, policy, "pack", "--output", relative_output)
    if not (output / "resources.db").is_file():
        raise SystemExit("nanres pack did not produce resources.db")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
