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
| `cli/main.c` | CLI argument parsing, module dispatch, diagnostics, and output |
| `tests/test_amcore.c` | Library calculations, invalid inputs, and range behavior |
| `tests/test_cli.cmake` | CLI integration checks |
| `CMakeLists.txt` | Targets, options, tests, install/export/package configuration |
| `.github/workflows/ci.yml` | Configure/build/test on Windows, macOS, and Linux |
| `README.md` | User build, install, CLI, and API guide |
| `doc/standards/IEEE 149-2021 Recommended Practice for Antenna Measurements.pdf` | Project reference copy of IEEE Std 149-2021 |

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
- Preserve portability: C99 implementation, no OS-specific dependencies, and
  no dynamic allocation in the computational library.

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

## Standards reference

The far-field-distance calculation is based on section 7.1.2 of *IEEE Std
149-2021, IEEE Recommended Practice for Antenna Measurements*. A project copy
is stored at
[`doc/standards/IEEE 149-2021 Recommended Practice for Antenna Measurements.pdf`](standards/IEEE%20149-2021%20Recommended%20Practice%20for%20Antenna%20Measurements.pdf).
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
5. Keep `README.md` as user-facing usage documentation and this file as the
   concise agent orientation. Update both when a change affects their subject
   matter.
6. Report validation accurately, including failures or validation not run.

## Historical design context

A chat on 2026-10-22 proposed **AMCore** (Antenna Measurement Core) as a name
for a modular C library, with `amcore_` as the public function prefix and
`amcore_`-prefixed public types and headers. This aligns with the repository's
current name and API conventions. The discussion also listed future areas:
chamber design, device control, near/far-field computation, pattern
processing, and post-processing. Use this as roadmap context only; the
checked-in implementation remains the source of truth for what exists.
