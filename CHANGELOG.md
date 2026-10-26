# Changelog

All notable changes for each release are documented here.

Entries under released versions are historical release notes and may describe
features that are not present in the current branch.

## [Unreleased]

## [v0.5.3] - 2026-10-26

### Added

- Far-field line-of-sight and radio-horizon estimates, plus free-space path
  loss calculations, in the C API and CLI.

## [v0.5.2] - 2026-10-26

### Changes

- chore: update version to 0.5.2 and modify CLI test outputs to reflect module terminology

## [v0.5.1] - 2026-10-26

### Changes

- chore: update version to 0.5.1 and enhance documentation with module descriptions and usage examples
- docs: enhance documentation with versioning support and add hooks for library version
- docs: update README and getting-started guide with documentation build instructions

## [v0.5.0] - 2026-10-26

### Changed

- Remove operation logging, telemetry, and CLI call history, including their
  implementation, build options, dependencies, and tests. Preserve the
  calculation API, CLI calculations, help, and version commands.
- Update the user, API, CMake, WebAssembly, and development documentation to
  describe the calculation-only branch.

## [v0.4.1] - 2026-10-23

### Changes

- feat: implement history command for CLI
- docs: update documentation to reflect changes in Read the Docs configuration and dependencies
- feat: add Read the Docs configuration for documentation build
- refactor: simplify CI configuration and update documentation on build processes

## [v0.3.0] - 2026-10-23

### Changes

- feat: update version to 0.3.0 in CMakeLists.txt and vcpkg.json
- chore: update last content update tags to v0.2.0 across documentation and source files
- feat: update CI and documentation workflows for release-tag-only builds and version checks
- feat: add machine-readable API manifest and update documentation
- feat: add versioning support to operation logging and telemetry features
- Implement telemetry and operation logging features
- docs: update CHANGELOG for v0.2.0 release

## [v0.2.0] - 2026-10-23

### Added

- Optional desktop SQLite operation logging with
  `AMCORE_ENABLE_OPERATION_LOGGING`, recording function inputs, status, output,
  UTC start time, and duration in per-function tables in the user's local
  `libamcore.db`.
- CLI help reports the compile-time logging setting and, when enabled, the
  database location template without changing calculation or version output.
- Logging integration tests, opt-in vcpkg SQLite dependency, and enabled/disabled
  CI coverage on Windows, Linux, and macOS.

### Changed

- Updated public build, API, CLI, and platform documentation, LLM context, and
  development skill guidance for operation logging and current project paths.
- Add optional desktop operation logging with SQLite support
- docs: enhance API documentation with LaTeX support for equations
- fix: correct LaTeX formatting for far-field distance equation in API documentation
- ci: remove unused docs_changed output and related logic

## [v0.1.1] - 2026-10-22

### Changes

- Bump version to 0.1.1 in CMakeLists.txt for testing workflow.
- Enhance CI/CD workflows: add version check, changelog update, and documentation deployment steps
- Enhance documentation and CI: update .gitignore, add mkdocs configuration, and create public documentation files
- Update documentation and add changelog for v0.1.0 release; include last content update tags across multiple files
- Enhance documentation: clarify far-field distance calculation formula in farfield.h
- Add VSCode extensions recommendations for C++ and CMake development

## [v0.1.0] - 2026-10-22

### Added

- Portable C99 `libamcore` library with free-space wavelength calculation.
- Far-field distance calculation based on IEEE Std 149-2021, section 7.1.2.
- `amcore` CLI commands for wavelength and far-field distance calculations,
  including help, version, and input-error reporting.
- CMake support for static and shared libraries, installation, and package
  consumption through `find_package(amcore CONFIG REQUIRED)`.
- Optional Emscripten WebAssembly build target.
- Unit and CLI integration tests, plus CI and release workflows.
- README build, API, CLI, installation, WebAssembly, and packaging guidance;
  AI development context and workflow documentation.
