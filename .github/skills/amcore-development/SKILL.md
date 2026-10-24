---
# Last content update: Unreleased (latest tag: v0.2.0)
name: amcore-development
description: "Develop and document libamcore, the portable C antenna-measurement library. Use for C API changes, computational modules, CLI behavior and call history, desktop SQLite operation logging, CMake builds, tests, or project documentation."
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
   allocation by default. Opt-in logging and telemetry may add allocation,
   clocks, and I/O. SQLite remains desktop logging-only; telemetry uses
   libwebsockets on desktop and Emscripten Fetch on WebAssembly. Follow the
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
   Update this skill when development workflows or feature contracts change,
   and record notable features in `CHANGELOG.md`. Keep version metadata in
   `CMakeLists.txt`, `vcpkg.json`, and `docs/public/api.json` synchronized when
   updating the project version.
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
  preserve nested-call records. Declare named typed input/output collections
  in each function with `AMCORE_OPERATION_FIELDS` and
  `AMCORE_OPERATION_BEGIN(inputs, count, outputs, count)`; use
  `AMCORE_OPERATION_RETURN(status)` on every exit. Both sinks consume the
  shared generic event; never add function-name branches or fixed argument
  slots. Extend shared field types for new data shapes. Keep deployed schemas
  stable or provide an explicit migration.
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
  build requires SQLite 3.24+ development files; with vcpkg, enable manifest feature
  `operation-logging`. Run `amcore_operation_log_tests` and relevant CLI tests;
  keep test user directories isolated from real user data. Check static/shared
  builds and installed dependency propagation when changing CMake wiring.
- Keep [CMake integration](../../../docs/public/cmake-integration.md) current
  for database paths, schema, timing boundaries, and retention limitations.

### CLI Call History

- `amcore history` exists only in logging-enabled builds; disabled builds
  must neither advertise nor recognize it. Keep calculation stdout numeric-only;
  history intentionally prints aligned, bordered tables with ID first.
- Reuse `src/operation_log_path.c` for the database location. History resolves
  the path without creating directories and opens SQLite read-only. It must
  not migrate schemas, record calls, send telemetry, or modify the database.
  Help must not access the database.
- Summary rows cover all function/version tables, including failed and nested
  calls. Sort by `executed_at_us` descending, table name ascending, then row
  ID descending. `-l`, `--limit`, and `--limit=<value>` set a global limit,
  defaulting to 10; accept only positive decimal integers up to INT64_MAX.
- IDs are local to each function/version table. `--function <name>` filters
  summaries across versions; `--function <name> --id <id>` displays every
  stored field in a Field/Value table for every matching version. Detail lookup
  requires both flags, rejects `--limit`, and is not capped at 10. Support
  `--function=<name>` and `--id=<id>` too.
- Derive fields from each table's schema, not hard-coded function branches.
  For historical tables lacking a `version` column, use the table's version
  suffix or `unknown` for unversioned tables. Preserve NULL outputs as `NULL`
  and escape controls, backslashes, and pipes as `\xHH` in displayed values.
  Do not rely on a compound SELECT across all tables: databases can exceed
  SQLite's compound-SELECT limit.
- Empty summaries print headers and borders. Missing detail matches and
  missing, unreadable, corrupt, or incompatible databases fail with stderr
  diagnostics and nonzero exit status; never hide query failures.
- Run `amcore_cli_history` using `tests/test_history.cmake` and
  `tests/history_fixture.c`. Cover alignment, IDs, limits, ordering, function
  filters, cross-version details, legacy schemas, NULL/escaped values, many
  version tables, read-only access, and invalid options. Keep fixtures isolated
  from real user data and verify history remains absent in disabled builds.

## Telemetry

- `AMCORE_ENABLE_TELEMETRY` is off by default and independent of SQLite logging.
  `AMCORE_TELEMETRY_ENDPOINT` is required when enabled; there is no default URL
  or runtime toggle. Keep it disabled on embedded targets, including Linux.
- Reuse `src/operation_log.h` instrumentation and shared `src/operation.c`
  timing. Capture duration before dispatching either sink, write SQLite first
  when enabled, then send telemetry. Preserve nested and failed calls and
  calculation behavior even when either sink fails.
- Serialize the documented JSON shape in `src/telemetry.c` with a bounded
  encoder validated by coreJSON. Telemetry builds fetch coreJSON v3.3.1 during
  configuration; `FETCHCONTENT_SOURCE_DIR_COREJSON` supports offline builds.
  Cap each request body at 1 MiB by default; CMake cache variable
  `AMCORE_TELEMETRY_MAX_BODY_SIZE_KB` configures the limit in KB (1 KB = 1024
  bytes). When needed, omit outputs first and inputs second; skip an event only
  if its remaining metadata
  still exceeds the cap, without changing calculation results. Preserve exact
  int64 literals and never accept raw user JSON. Failed output is
  null, non-finite inputs are strings, and pointers/user identifiers are never
  sent. Report errors to stderr with `amcore telemetry:`.
- Generic fields support double, int64_t, int boolean and UTF-8 text; null
  pointers are values, not addresses. Descriptors and typed storage must live
  through operation return. Snapshot mutated inputs at entry. Use
  `tests/test_operation_fields.c` to verify new shared types, escaping, large
  events and multiple/empty field collections.
- Desktop transport is libwebsockets 4.3+ (vcpkg feature `telemetry`), with TLS
  verification enabled, optional `AMCORE_TELEMETRY_CA_FILE`, no redirects, and
  a five-second event-loop deadline. Do not add Mongoose.
- WebAssembly uses synchronous Emscripten Fetch on pthread worker threads;
  reject browser-main-thread transmission explicitly without changing results.
  Telemetry builds require JS glue, pthreads, cross-origin isolation and CORS.
  Preserve the standalone WASM output when telemetry is disabled.
- Test enabled and disabled modes and both together with SQLite. Telemetry
  CTest requires Node.js and a loopback endpoint such as
  `http://127.0.0.1:18765/events`; do not send test data to remote services.
  Validate payloads, timing, failure semantics, CLI help, shared/static builds
  and installed CMake dependency propagation.

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
