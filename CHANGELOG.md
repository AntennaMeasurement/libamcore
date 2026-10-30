# Changelog

All notable changes for each release are documented here.

Entries under released versions are historical release notes and may describe
features that are not present in the current branch.

## [Unreleased]

### Added

- Add `amcore_near_field_cylindrical_ideal_y_step_size_m` to the C API,
  CLI (`nearfield cylindrical ideal-y-step-size`), and WebAssembly exports.
  It returns half a wavelength in metres without rounding, including
  submillimetre steps. The unused Python scan-size argument is omitted.

## [v0.8.7] - 2026-10-29

### Changed

- Rename the cylindrical ideal Y scan length API to
  `amcore_near_field_cylindrical_ideal_y_scan_length_m`.


## [v0.8.6] - 2026-10-29
- feat: add Emscripten setup and WASM build step for Linux release

## [v0.8.5] - 2026-10-29


## [v0.8.4] - 2026-10-29


## [v0.8.3] - 2026-10-29
- chore: add CI workflow and release script

## [v0.8.2] - 2026-10-28


## [v0.8.1] - 2026-10-28
- chore: update CHANGELOG and versioning in CMakeLists.txt for v0.8.0 release

## [v0.8.0] - 2026-10-28

### Changes

- chore: update project version to 0.8.0 and add release script with tests
- chore: remove CI and documentation workflows, and associated tests
- chore: update project version to 0.7.3, enhance CI workflows, and improve documentation
- ci: remove test step from CI workflow and update WebAssembly documentation

## [v0.7.3] - 2026-10-28

### Changes

- Update project version to 0.7.3 in CMakeLists.txt

## [v0.7.2] - 2026-10-28

### Changes

- Update project version to 0.7.2 and enhance description in CMakeLists.txt
- Add shared documentation dependencies
- Initial plan
- Fix CLI missing-argument failures
- Initial plan

## [v0.7.1] - 2026-10-27

### Added

- Add `amcore_near_field_cylindrical_y_scan_length_m` to the C API and
  WebAssembly exports, using NSI 2000 cylindrical scan geometry. Accept
  azimuth in [0, 180] degrees and elevation in [0, 90) degrees without a
  warning above 60 degrees. Test against the NSI2000 reference scan length
  of 11.186 m.

### Changed

- Print a PASS/FAIL list and summary from `amcore_tests`, running all tests
  instead of stopping at the first failure.
- Default Windows installations to `%LOCALAPPDATA%\Programs\libamcore` to
  avoid requiring administrator permissions.

## [v0.6.1] - 2026-10-26

### Changes

- chore: update version to 0.6.1 and improve README clarity
- docs: update references to IEEE standards and clarify descriptions

## [v0.6.0] - 2026-10-26

### Added

- Added planar and spherical near-field calculations to the C API, CLI, and
  documentation, including the recommended five-wavelength probe separation.

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
