---
# Last content update tag: v0.1.0
name: amcore-development
description: "Develop and document libamcore, the portable C antenna-measurement library. Use for C API changes, computational modules, CLI behavior, desktop SQLite operation logging, CMake builds, tests, or project documentation."
---

# AMCore Development

Use this skill when implementing or reviewing changes in this repository. For
the current architecture, implemented features, build options, and historical
project intent, read [the LLM project context](../../../docs/public/LLM_CONTEXT.md).
Confirm details against the current source; the context document is a guide,
not a substitute for code.

## Workflow

1. Read the relevant public header, implementation, and tests before editing.
   Check `README.md` and `CMakeLists.txt` when public behavior or build wiring
   is involved.
2. Keep calculations portable C99, with no OS-specific dependencies or dynamic
   allocation by default. Only the opt-in desktop logging backend may use
   SQLite, allocation, native clocks, and filesystem access. Follow the
   existing module naming and public-header patterns.
3. For numeric APIs, validate pointers and input domains, detect non-finite or
   unrepresentable results, return the appropriate `amcore_status`, and write
   output parameters only on success. Avoid overflow-prone intermediate
   calculations.
4. Add or update focused tests for normal values, invalid inputs, range
   boundaries, and unchanged outputs on error, as appropriate.
5. Update `README.md`, affected pages in `docs/public/`, and
   `docs/public/LLM_CONTEXT.md` when user-visible behavior, supported modules,
   or build instructions change. Keep CLI help and build options consistent.
6. Configure, build, and run tests using the repository's CMake workflow:

   ```sh
   cmake -S . -B build -DBUILD_TESTING=ON
   cmake --build build --config Release
   ctest --test-dir build -C Release --output-on-failure
   ```

   If only a narrow surface changed, run the smallest relevant validation and
   state clearly what was and was not tested.

## Desktop Operation Logging

- `AMCORE_ENABLE_OPERATION_LOGGING` is off by default and supported only on
  desktop Windows, Linux, and macOS. Keep it disabled for embedded targets
  (including embedded Linux) and WebAssembly; do not add dependencies to the
  disabled build.
- Reuse `src/operation_log.h` instrumentation and `src/operation_log.c` when
  changing calculations. Instrument every return path, including failures;
  preserve nested-call records. Extend the backend's per-function schema and
  bindings explicitly for new functions rather than assuming existing columns
  fit new APIs.
- Store named numeric inputs, output-pointer nullness, status, successful
  output, UTC epoch-microsecond start time, and monotonic nanosecond duration.
  Store failed outputs as SQL NULL and non-finite inputs as text. Never read
  output contents on failure or persist pointer addresses.
- Report logging errors to stderr with the existing prefix without changing
  calculation results, return statuses, output-on-error semantics, or CLI exit
  codes. Keep calculation stdout numeric-only.
- CLI help reports the compile-time setting and enabled location template.
  There is no runtime toggle. CLI parsing errors and help/version do not reach
  the calculation API and are not logged; out-of-range library calls are.
- For logging changes, validate both enabled and disabled builds. The enabled
  build requires SQLite development files; with vcpkg, enable manifest feature
  `operation-logging`. Run `amcore_operation_log_tests` and relevant CLI tests;
  keep test user directories isolated from real user data. Check static/shared
  builds and installed dependency propagation when changing CMake wiring.
- Keep [CMake integration](../../../docs/public/cmake-integration.md) current
  for database paths, schema, timing boundaries, and retention limitations.

## API and Module Conventions

- Public headers live in `include/amcore/`; `<amcore/amcore.h>` is the umbrella
  header and includes the module headers.
- Public functions use the `amcore_` prefix, shared public declarations use
  `AMCORE_API`, and C++ consumers are supported through `extern "C"`.
- Keep units explicit in names and documentation (for example, `_hz` and
  `_m`).
- Return `amcore_status`; use the existing status values rather than inventing
  undocumented error handling.
- The CLI is a separate consumer of the C API. Preserve its module/subcommand
  structure, numeric output and error-to-exit-status behavior.
- Do not describe a planned antenna-measurement area as implemented unless
  there is code and a public interface for it.
