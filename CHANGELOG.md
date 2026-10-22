# Changelog

All notable changes for each release are documented here.

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
