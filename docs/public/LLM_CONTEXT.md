<!-- Last content update: Unreleased (latest tag: v0.2.0) -->
# libamcore: LLM Project Context

This document is a source-grounded orientation for AI coding agents and other
automated project workflows. Treat checked-in code and tests as authoritative
if this guide becomes stale.

## Purpose and current scope

`libamcore` is a portable C library of core antenna-measurement calculations.
This branch contains the calculation library, CLI, CMake packaging, tests, and
optional standalone WebAssembly build. Operation logging, telemetry, and CLI
call history are not included.

Implemented calculations:

- Free-space wavelength from frequency using the defined speed of light in
  vacuum.
- Minimum far-field distance using *IEEE Std 149-2021, IEEE Recommended
  Practice for Antenna Measurements*, section 7.1.2:
  `max(5D, 1.6 lambda, 2D^2/lambda)`.
- Approximate line-of-sight range and radio horizon from height above ground.
- Free-space path loss under far-field conditions, using frequency in MHz,
  range in km, and antenna gains in dB.
- Planar near-field sampling spacing, scan length, angle of view, and centered
  sampling parameters.
- Recommended five-wavelength near-field separation distance.
- Spherical near-field maximum radius extent.

Chamber design, device control, pattern processing, and post-processing are
not currently exposed.

## Source map

| Path | Responsibility |
| --- | --- |
| `include/amcore/amcore.h` | Umbrella public header. |
| `include/amcore/utility.h` | Status enum, export macro, speed-of-light constant, wavelength API. |
| `include/amcore/farfield.h` | Far-field calculation public API. |
| `include/amcore/nearfield.h` | Near-field calculation public API. |
| `src/utility.c` | Wavelength computation and physical constant. |
| `src/farfield.c` | Far-field distance, line-of-sight, radio-horizon, and path-loss computations. |
| `src/nearfield.c` | Recommended separation distance, planar near-field sampling and scan geometry, and spherical maximum-radius extent. |
| `cli/main.c` | CLI parsing, module dispatch, diagnostics, and output. |
| `tests/test_amcore.c` | Library calculations, invalid inputs, and range behavior. |
| `tests/test_cli.cmake` | CLI integration checks. |
| `CMakeLists.txt` | Targets, options, tests, install/export/package configuration. |
| `README.md` | User build, install, CLI, and API guide. |
| `docs/public/api.json` | Machine-readable metadata for the public calculation API. |
| `docs/public/` | Public MkDocs pages and this agent context (excluded from the site). |
| `.github/skills/amcore-development/SKILL.md` | Reusable development workflow. |

## Public API and behavior

- Public headers are under `include/amcore/`; `<amcore/amcore.h>` includes
  module headers.
- Public functions use the `amcore_` prefix and return `amcore_status`.
- Inputs must be finite and within each function's domain. Output pointers must
  be non-null and are modified only on success.
- `AMCORE_STATUS_SUCCESS`, `AMCORE_STATUS_INVALID_ARGUMENT`, and
  `AMCORE_STATUS_OUT_OF_RANGE` are the defined status values.
- The CLI is a separate C API consumer. Calculation output is numeric-only on
  stdout; errors are written to stderr and return a nonzero exit status.
- The CLI provides `help`, `version`, `utility wavelength`, the far-field
  commands `distance`, `line-of-sight`, `radio-horizon`, and `path-loss`, and
  near-field planar and spherical calculations. It does not provide a
  `history` command.

The machine-readable manifest documents functions, input schemas, units,
status codes, and examples. It is not an HTTP API or server implementation.

## Build and validation

Requirements are CMake 3.15 or newer and a C99-capable compiler. The default
configuration builds a static library and CLI. Shared libraries and a
standalone Emscripten WebAssembly target are optional.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For changes to the public API or build wiring, check both the library and CLI
tests. Keep CMake options and user documentation consistent.

## Coding conventions

- Keep calculations portable C99, without OS-specific dependencies or dynamic
  allocation by default.
- Validate input pointers and domains, detect non-finite or unrepresentable
  results, and preserve output parameters on errors.
- Use `AMCORE_API` for shared public declarations. C++ consumers are supported
  through `extern "C"`.
- Keep units explicit in names and documentation, such as `_hz` and `_m`.
- Do not describe planned antenna-measurement areas as implemented unless code
  and a public interface exist.

See [the development skill](../../.github/skills/amcore-development/SKILL.md)
for the repository workflow.
