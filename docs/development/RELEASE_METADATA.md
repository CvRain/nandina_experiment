# Release Metadata Contract

This document defines the machine-readable release metadata used while the C++26 line is still
pre-1.0. It keeps the SDK version, Meson project version, source-bundle defaults, CLI compatibility,
support profile, and release limitations reviewable from one file.

`release-metadata.toml` at the repository root is the source of truth for release identity. The
current experiment is intentionally unreleased, so its revision is recorded as `unreleased` until
C7 freezes an RC commit and tag.

The framework SDK and `NandinaCLI` have independent versions. `minimum_cli` expresses the lowest
CLI that understands this SDK's source/index contract; it is not the CLI's own release number.

The metadata checker validates these fields against `meson.build`, `tools/sdk_bundle.py`,
`CHANGELOG.md`, and the acceptance contract. It is a review gate, not a substitute for choosing a
license or creating an RC tag.
