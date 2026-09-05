# Changelog

## Unreleased (pre-1.0)

The C++26 experiment line currently targets a Linux desktop source-consumption release. The C6
quality and source-distribution work is complete: GCC/Clang CI, sanitizer coverage, SQLite fallback,
physics configuration, reproducible SDK source archives, offline consumers, and relocatable portable
staging are verified.

The remaining 1.0 closure work is the second-device Linux/manual gate, final RC metadata and review,
and promotion into the official `NandinaUI` repository. Framework installation into system or XDG
prefixes, native application packages, and Windows/macOS support are deliberately not promised.

Known limitations and the supported compiler/toolchain baseline are maintained in
`docs/development/1.0_ACCEPTANCE.md` and `docs/development/LINUX_PLATFORM.md`.
