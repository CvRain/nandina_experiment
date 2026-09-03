# Nandina CLI And SDK Distribution Contract

## 1. Repository Roles

- `NandinaCLI` is the independent command-line product. It owns project creation, source
  configuration, SDK resolution/cache, build, run, and diagnostics. It must build without a
  NandinaUI checkout and must not compile an absolute NandinaUI source path into the executable.
- `NandinaUI` owns framework/runtime source, `nanres`, SDK-compatible templates, and official SDK
  artifacts.
- `nandina_experiment` validates architecture and 2.0 experiments; it is not a production package
  origin.

CLI and SDK versions are independent. Updating the CLI never silently changes a project's SDK.

## 2. Project Contract

A generated application contains:

- `nandina.toml`: human-maintained project identity and SDK requirement;
- `nandina.lock`: tool-owned exact source, revision/artifact, and SHA-256;
- `subprojects/nandina.wrap`: generated Meson interoperability input;
- `resources/resources.toml`: the existing human-maintained resource inventory.

The default release path must not create `subprojects/nandina` as a symlink. An explicit local-path
provider may use a platform-appropriate development link or copy, but it is never the published
project default.

## 3. Source And Mirror Model

`sources.toml` uses two distinct concepts:

- a source/registry publishes package identities, versions, channels, compatibility metadata, and
  artifact hashes;
- a mirror provides alternate transport for the exact same hashed artifact.

Configuration is layered in this order: built-in defaults, system, user, project, command line.
Higher layers may add, disable, or explicitly select sources. They may not silently redefine the
built-in official trust identity.

Supported provider kinds are introduced in this order:

1. `registry`: a static signed index plus archive artifacts; the release default;
2. `git`: an explicit fork/nightly source resolved to an immutable commit in `nandina.lock`;
3. `path`: an explicit local framework-development override;
4. `binary`: a future platform/triplet-specific prebuilt SDK.

Once a project names a source, resolution does not fall through to a different source. Mirrors may
fall through only when the fetched bytes match the locked SHA-256.

## 4. Resolution And Network Rules

- `new`, `sdk search`, `sdk fetch`, `sdk update`, and `source refresh` may refresh source metadata.
- `build --locked` consumes the existing lock and never changes source, version, or revision.
- `build --offline` reads only verified cache entries.
- A floating Git ref is resolved to a commit before the project is considered locked.
- Unknown project-declared sources require explicit trust; non-interactive use must opt in.
- Downloads are written to temporary files, size-limited, SHA-256 verified, and atomically
  published into a content-addressed cache.
- Archive path traversal, cache corruption, concurrent writers, interrupted downloads, proxy
  configuration, and mirror failures must produce actionable diagnostics.

The initial registry protocol may be a static TOML/JSON index served by ordinary HTTPS. Official
indexes are signed with a key whose identity ships with the CLI. Custom registries may configure a
different key; unsigned registries require an explicit insecure/trust decision.

## 5. SDK Artifact Contract

Platform-generated repository archives are not release SDKs because Git submodule content is not
guaranteed to be included. NandinaUI release CI publishes a deliberate SDK source bundle containing
the framework, `nanres`, compatible templates, required dependency sources or pinned wraps,
licenses, and an SDK manifest.

Each release publishes at least:

- a Meson/Python 3.11-compatible `.tar.xz` registry artifact for Linux 1.0;
- a `.tar.zst` source bundle for newer Unix-like tooling;
- a `.zip` source bundle for Windows;
- hashes/signatures and a machine-readable SDK manifest;
- the source-index entry consumed by NandinaCLI.

The first 1.0 implementation remains source-first. Prebuilt SDKs may be added later without
changing the project manifest or lock model.

NandinaUI 1.0 does not publish a system-installed framework SDK or native application packages.
There is no supported installation of framework headers/libraries into `/usr`, `/usr/local`, or
`~/.local`, and no deb/rpm/Flatpak/Snap/AppImage, macOS bundle, or Windows installer contract.
Instead, the generated application produces a relocatable staging tree that downstream packagers
may translate into their platform format. `resources.db` and its relative `external/` directory are
copied together; build-only `resource-location.json` is excluded.

## 6. CLI Delivery And Platform Boundary

NandinaCLI uses C++20 so building the tool does not itself require the C++26 application toolchain.
CLI binary packaging is independent from the NandinaUI 1.0 SDK/application distribution promise;
source builds remain supported.
The C++26 compiler/library capability remains a `nandina doctor` application-build check.

Process execution, configuration/cache directories, locking, atomic publication, executable
suffixes, and archive handling live behind platform boundaries. POSIX `fork`/`exec` is not exposed
to command logic and is not the Windows implementation.

## 7. C2.1 Acceptance

Implementation status: `NandinaCLI` commits `3716bbd`, `a6722f9`, `742c02d`, `3f33ba7`, and
`71f27f4` complete the independent C++20 repository, registry/archive provider, layered and
editable source configuration, size/SHA-256 verified cache, pinned project generation, locked
`build`/`run`/`doctor` parity, Git fork resolution to exact commits, explicit path development mode,
Ed25519 detached-index verification, and platform-selected process/config/cache/path backends.
C2.1's code contract is complete. Native Windows/macOS support claims remain future platform work;
provisioning the built-in official release key belongs to the C6 release-key ceremony.

- The independent repository configures, builds, and tests without a NandinaUI checkout.
- A local static registry fixture resolves an SDK, verifies its hash, and reuses the verified cache.
- Source priority, explicit selection, disabled entries, malformed indexes, and hash mismatch are
  tested.
- Generated release-mode projects contain a pinned wrap/lock and no source-tree symlink.
- The local-path provider remains covered as an explicit development mode.
- CLI source configuration, cache, and subprocess paths have platform-neutral interfaces before
  Windows/macOS backends are claimed.
