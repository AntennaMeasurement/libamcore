<!-- Last content update tag: v0.1.0 -->
# libamcore: LLM Project Context

This document is a compact, source-grounded orientation for AI coding agents
and other automated project workflows. Treat checked-in code and tests as
authoritative when this guide becomes stale.

## Purpose and project intent

`libamcore` is a portable C library of core computational routines for antenna
measurement. The project is intended to grow as modular C functionality. A
prior project-design conversation described a broader goal that includes
chamber design, device control, near- and far-field computations, pattern
processing, and post-processing. Those are **project goals, not claims about
the current implementation**.

## Implemented today

- Free-space wavelength from frequency, using the defined speed of light in
  vacuum.
- Minimum far-field distance using *IEEE Std 149-2021, IEEE Recommended
  Practice for Antenna Measurements*, section 7.1.2:
  `max(5D, 1.6 lambda, 2D^2/lambda)`.
- A small `amcore` CLI that exposes those calculations.
- Static library builds by default, optional shared-library builds, CMake
  installation/package support, opt-in C tests, and an optional Emscripten
  WebAssembly target.
- Opt-in desktop SQLite operation logging for every public calculation call,
  including failed and nested calls, without changing the public C API.

Near-field calculations, chamber design, device control, pattern processing,
and post-processing are not currently exposed by the library. Confirm against
the current headers before making claims about feature availability.

## Source map

| Path | Responsibility |
| --- | --- |
| `include/amcore/amcore.h` | Umbrella public header |
| `include/amcore/utility.h` | Status enum, export macro, speed-of-light constant, wavelength API |
| `include/amcore/farfield.h` | Far-field distance public API |
| `src/utility.c` | Wavelength computation and physical constant |
| `src/farfield.c` | Far-field distance computation |
| `src/operation_log.h`, `src/operation_log.c` | Compile-time instrumentation and optional desktop SQLite backend |
| `cli/main.c` | CLI argument parsing, module dispatch, diagnostics, and output |
| `tests/test_amcore.c` | Library calculations, invalid inputs, and range behavior |
| `tests/test_cli.cmake` | CLI integration checks |
| `tests/test_operation_log.c`, `tests/test_operation_log.cmake` | SQLite records, timing, persistence, paths, and logging failure behavior |
| `CMakeLists.txt` | Targets, options, tests, install/export/package configuration |
| `.github/workflows/ci.yml` | Configure/build/test with logging enabled and disabled on Windows, macOS, and Linux |
| `README.md` | User build, install, CLI, and API guide |
| `docs/public/` | Public MkDocs pages and this agent context (excluded from the site) |
| `.github/skills/amcore-development/SKILL.md` | Reusable development workflow and logging conventions |
| `docs/standards/IEEE 149-2021 Recommended Practice for Antenna Measurements.pdf` | Project reference copy of IEEE Std 149-2021 |

## Public API contract

- Include `<amcore/amcore.h>` for the full currently supported API, or an
  individual module header as needed.
- The public API is C-compatible and also supports C++ callers through
  `extern "C"`.
- Status results are `AMCORE_STATUS_SUCCESS`,
  `AMCORE_STATUS_INVALID_ARGUMENT`, and `AMCORE_STATUS_OUT_OF_RANGE`.
- Frequency parameters are in hertz; aperture lengths, wavelengths, and
  distances are in metres. Inputs must be finite and positive.
- Numeric functions return a status and write the result through an output
  pointer only on success. On error the caller's output value remains
  unchanged.
- Use explicit unit suffixes in new API names and document units, constraints,
  output behavior, and error statuses in public headers.
- Preserve portability: C99 implementation with no OS-specific dependencies or
  dynamic allocation by default. Only the optional desktop logging backend
  uses allocation, native clocks/filesystems, and SQLite.

## Build, test, and CLI

Requirements are CMake 3.15 or newer and a C99-capable compiler. Configure,
build, and test:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Useful configure options:

- `-DBUILD_SHARED_LIBS=ON`: build a shared library instead of the default
  static library.
- `-DAMCORE_BUILD_CLI=OFF`: omit the CLI, useful for firmware/cross builds.
- `-DBUILD_TESTING=ON`: build and register tests (off by default).
- `-DAMCORE_ENABLE_OPERATION_LOGGING=ON`: enable synchronous SQLite logging on
  desktop Windows, Linux, and macOS; requires SQLite development files (vcpkg
  manifest feature `operation-logging`, selected with
  `-DVCPKG_MANIFEST_FEATURES=operation-logging` and the vcpkg CMake toolchain).
  The manifest pins `builtin-baseline` for registry version resolution;
  keep it compatible with the vcpkg tool's scripts when updating dependencies.
  Supply the toolchain on the first build-directory configuration; otherwise
  use a new directory or `cmake --fresh` (CMake 3.24+) with all required options.
  Rejected for WebAssembly and non-desktop target systems; leave off for
  embedded Linux. Stores `libamcore/libamcore.db` in the
  user's local application-data directory, with a table per function, named
  inputs, output-pointer nullness, return status, successful output, UTC
  epoch-microsecond start time, and monotonic nanosecond duration. Failed
  outputs are SQL NULL and non-finite inputs are text. Database errors go to
  stderr without altering calculation behavior. Disabled builds require no
  SQLite. See `README.md` for paths, schema, timing, and retention limitations.
- `-DAMCORE_BUILD_WASM=ON`: build the optional standalone WebAssembly target
  with the Emscripten toolchain; CLI and tests should be disabled in that
  configuration as described in `README.md`.

Current CLI commands:

```text
amcore utility wavelength <frequency_hz>
amcore farfield distance <frequency_hz> <aperture_length_m>
```

Success prints one numeric result in metres to standard output. Invalid
arguments and out-of-range computations emit a diagnostic to standard error
and return a nonzero exit code. See `README.md` for complete examples,
installation, packaging, and cross-compilation instructions.

General, module, and calculation help display the compile-time logging setting
and, when enabled, the platform database location template. Version output
remains `amcore <version>`; there is no runtime logging toggle.
`AMCORE_CLI_OPERATION_LOGGING` is a private CMake definition matching the
library's build setting, not a public API macro.
CLI numeric parsing and syntax failures never call the library, so they are
not recorded. Out-of-range library calls are recorded. Logging errors can
appear on stderr alongside a successful calculation without changing its
numeric stdout or zero exit code. Help/version do not create database records.

## Logging implementation and validation

`src/operation_log.h` compiles instrumentation away when disabled. Enabled
calculations time every exit path and pass named inputs and status to the
desktop backend. The backend explicitly handles the current two functions;
extend its schema and bindings when adding APIs. Never read an output value on
failure or store pointer addresses. Nested wavelength calls are recorded only
when the far-field function reaches that calculation.

The database belongs to the library's CMake project (`libamcore.db`), not its
consuming application. Directories are `%LOCALAPPDATA%\libamcore` on Windows,
`$XDG_DATA_HOME/libamcore` or `$HOME/.local/share/libamcore` on Linux, and
`$HOME/Library/Application Support/libamcore` on macOS. Missing directories are
created; relative or absent Linux XDG locations fall back to HOME. No fallback
to the working directory is permitted.

Each row stores `id`, `frequency_hz`, optional `aperture_length_m`,
`output_is_null`, `return_value`, `wavelength_m` or `distance_m`,
`executed_at_us`, and `duration_ns`. Status values are 0 (success), 1 (invalid
argument), and 2 (out of range). Non-finite inputs are text `NaN`, `+Infinity`,
or `-Infinity`. Durations exclude the call's own write but include nested
logging. Writes are synchronous with one connection per record and five-second
SQLite lock waits per operation. There is no buffering, retry, or retention
policy; logging is unsuitable for real-time workloads.

For relevant changes, build/test both logging modes and preserve user-data
isolation in CTest. `tests/test_operation_log.cmake` exercises persistence,
Unicode paths, errors, and Linux XDG fallback; the C tests verify stored values,
statuses, NULL outputs, timestamps, and durations.
`tests/test_cli.cmake` verifies logging help in both modes, numeric-only
calculation stdout, and unchanged results/exit status on a logging error.
Check installed CMake consumers when changing dependency wiring. SQLite
development files are needed only when enabled; dynamically linked SQLite must
be distributed separately.

## Standards reference

The far-field-distance calculation is based on section 7.1.2 of *IEEE Std
149-2021, IEEE Recommended Practice for Antenna Measurements*. A project copy
is stored at
[`docs/standards/IEEE 149-2021 Recommended Practice for Antenna Measurements.pdf`](../standards/IEEE%20149-2021%20Recommended%20Practice%20for%20Antenna%20Measurements.pdf).
See the [IEEE Xplore record](https://ieeexplore.ieee.org/document/9597348)
for publication details and availability. Cite the standard and relevant
section when documenting calculations based on it; do not treat the reference
copy as evidence that other standard procedures are implemented.

## Change guidance for AI agents

1. Inspect the exact public header, implementation, tests, and CMake wiring
   before changing a module.
2. Keep a module coherent: public declaration in `include/amcore/`, source
   implementation in `src/`, and tests in `tests/`; wire any new source,
   header, CLI command, and documentation into their consumers explicitly.
3. For numerical work, test representative values plus invalid and extreme
   values. Guard overflow/underflow and preserve output-on-error semantics.
4. Prefer focused changes that preserve existing CLI/API behavior. Avoid
   adding dependencies or changing established behavior without an explicit
   need and corresponding tests/docs.
5. Keep `README.md`, affected public pages, CLI help, this context, and the
   development skill aligned when a change affects their subject matter.
6. Report validation accurately, including failures or validation not run.

## Historical design context

A chat on 2026-10-22 proposed **AMCore** (Antenna Measurement Core) as a name
for a modular C library, with `amcore_` as the public function prefix and
`amcore_`-prefixed public types and headers. This aligns with the repository's
current name and API conventions. The discussion also listed future areas:
chamber design, device control, near/far-field computation, pattern
processing, and post-processing. Use this as roadmap context only; the
checked-in implementation remains the source of truth for what exists.
