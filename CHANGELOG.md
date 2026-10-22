# Changelog

All notable changes for each release are documented here.

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
