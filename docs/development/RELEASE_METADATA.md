# Release Metadata Contract

This document defines the machine-readable release metadata used by the `0.1.0-alpha.1` C++26 line.
It keeps the SDK version, Meson project version, source-bundle defaults, CLI compatibility,
support profile, and release limitations reviewable from one file.

`release-metadata.toml` at the repository root is the source of truth for release identity. The
current alpha is intentionally unreleased, so its revision is recorded as `unreleased` until the
alpha tag is created. The alpha is an interim source-consumption checkpoint, not the 1.0 RC.

The framework SDK and `NandinaCLI` have independent versions. `minimum_cli` expresses the lowest
CLI that understands this SDK's source/index contract; it is not the CLI's own release number.

The metadata checker validates these fields against `meson.build`, `tools/sdk_bundle.py`,
`CHANGELOG.md`, the root `LICENSE`, and the acceptance contract. It is a review gate, not a
substitute for creating an RC tag.
