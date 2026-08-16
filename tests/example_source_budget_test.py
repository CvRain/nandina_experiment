#!/usr/bin/env python3

from pathlib import Path
import sys


root = Path(sys.argv[1])
bootstrap = root / "example" / "settings_example" / "settings_main.cpp"
application = [
    bootstrap,
    root / "example" / "settings_example" / "settings_example.hpp",
    root / "example" / "settings_example" / "settings_example.cpp",
]


def nonblank_lines(path: Path) -> int:
    return sum(bool(line.strip()) for line in path.read_text(encoding="utf-8").splitlines())


bootstrap_lines = nonblank_lines(bootstrap)
application_lines = sum(nonblank_lines(path) for path in application)
if bootstrap_lines > 30:
    raise SystemExit(f"example bootstrap exceeds budget: {bootstrap_lines} > 30")
# Budget raised from 220 to 260 in Step 6 (brand theme reference scales, data),
# then to 280 for component visual-variant showcases, and to 300 for the scrollable
# full Settings showcase (Tabs/Select/Tooltip/Basics) the user requested to verify.
if application_lines > 300:
    raise SystemExit(f"settings example exceeds budget: {application_lines} > 300")

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
        raise SystemExit(f"settings example uses forbidden application plumbing: {forbidden}")

print(f"example source budgets: bootstrap={bootstrap_lines}, application={application_lines}")
