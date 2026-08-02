#!/usr/bin/env python3

from pathlib import Path
import sys


root = Path(sys.argv[1])
bootstrap = root / "example" / "compact_main.cpp"
application = [
    bootstrap,
    root / "example" / "compact_todo.hpp",
    root / "example" / "compact_todo.cpp",
]


def nonblank_lines(path: Path) -> int:
    return sum(bool(line.strip()) for line in path.read_text(encoding="utf-8").splitlines())


bootstrap_lines = nonblank_lines(bootstrap)
application_lines = sum(nonblank_lines(path) for path in application)
if bootstrap_lines > 30:
    raise SystemExit(f"compact bootstrap exceeds A14 budget: {bootstrap_lines} > 30")
if application_lines > 300:
    raise SystemExit(f"compact Todo exceeds A14 budget: {application_lines} > 300")

source = "\n".join(path.read_text(encoding="utf-8") for path in application)
for forbidden in (
    '"scene/scene_tree.hpp"',
    '"scene/frame_scheduler.hpp"',
    '"app/ui_dispatcher.hpp"',
    '"reactive/scope.hpp"',
    "on_process(",
    "on_layout(",
    "on_draw(",
    "on_theme_changed(",
    "NanPageT<",
    "route_key(",
):
    if forbidden in source:
        raise SystemExit(f"compact Todo uses forbidden application plumbing: {forbidden}")

print(f"A14 source budgets: bootstrap={bootstrap_lines}, application={application_lines}")
