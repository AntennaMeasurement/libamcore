<!-- Last content update: Unreleased (latest tag: v0.2.0) -->
# libamcore

`libamcore` is a portable C library for core computational routines used in
antenna measurements. It builds as a static library by default for embedded use, can
also be built as a shared library. By default it performs no dynamic allocation
and has no operating-system or platform-specific dependencies. Optional desktop
operation logging adds SQLite and native filesystem/clock dependencies.
Independent, opt-in telemetry adds HTTP delivery using libwebsockets on desktop
or Emscripten Fetch on WebAssembly. Both features are disabled by default.

## Modules and Functions

<!-- Modules and Functions table -->

| Module     | Function                       | Description                                                                       |
| ---------- | ------------------------------ | --------------------------------------------------------------------------------- |
| `utility`  | `amcore_free_space_wavelength` | Computes the free-space wavelength for a given frequency                          |
| `farfield` | `amcore_far_field_distance`    | Computes the minimum far-field distance for a given frequency and aperture length |

## Programming API

The public API is available through `<amcore/amcore.h>`, which includes the
utility module's `<amcore/utility.h>` API and the far-field module's
`<amcore/farfield.h>` API. The utility module exports
`speed_of_light_m_per_s` as a shared constant and provides
`amcore_free_space_wavelength`. `AMCORE_API` exports symbols from shared builds
on Windows and platforms with GCC-compatible visibility attributes. Consumers
linking through the CMake target receive the matching import/export settings.

For telemetry receivers and MCP/server adapters, [the machine-readable API
manifest](docs/public/api.json) describes the current functions, JSON input
schemas, units, status codes, examples, and telemetry event fields. It is
metadata, not a server implementation or a new wire protocol. See the
[integration notes](docs/public/api.md#machine-readable-manifest).


## Building

Requirements: CMake 3.15 or newer and a C99-capable C compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Static builds are the default. To build a shared library instead, configure
with `-DBUILD_SHARED_LIBS=ON`:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON
cmake --build build-shared --config Release
```

### Desktop operation logging

Configure with `-DAMCORE_ENABLE_OPERATION_LOGGING=ON` to persist every public
calculation call, including invalid inputs and nested library calls:

```sh
cmake -S . -B build-logging -DAMCORE_ENABLE_OPERATION_LOGGING=ON
cmake --build build-logging --config Release
```

This option is **off by default** and supports desktop Windows, Linux, and
macOS only. WebAssembly/Emscripten and other target systems (including bare-metal
embedded toolchains) are rejected at configuration time. Leave it off for
embedded builds, including embedded Linux. Disabled builds contain no logging
code and require no SQLite dependency.

Install SQLite 3.24+ development headers and libraries before configuring; CMake
uses `find_package(SQLite3)`. To use the repository's vcpkg manifest, configure
with the vcpkg toolchain and enable the `operation-logging` manifest feature.
See [vcpkg](#vcpkg) for a complete command. Installed CMake packages resolve
SQLite for consumers automatically. SQLite must also be available at runtime
when linked dynamically; it is not bundled in the libamcore package.

The database is named `libamcore.db` (from the library's CMake project name,
not the consuming application's name) and is created on the first call:

| Platform | Database path |
| --- | --- |
| Windows | `%LOCALAPPDATA%\libamcore\libamcore.db` |
| Linux | `$XDG_DATA_HOME/libamcore/libamcore.db`, or `~/.local/share/libamcore/libamcore.db` when XDG_DATA_HOME is unset, empty, or relative |
| macOS | `~/Library/Application Support/libamcore/libamcore.db` |

Missing directories are created automatically. Windows paths support Unicode.
The relevant environment variables must resolve to an absolute user directory;
there is no fallback to the current working directory.

Tables are named `amcore_free_space_wavelength_<version>` and
`amcore_far_field_distance_<version>`, where `<version>` is the libamcore
project version (for example, `amcore_free_space_wavelength_0.2.0`). Each
release writes to its own tables; existing tables, including the unversioned
tables created by older releases, are left unchanged. Each row includes a
`version` text column containing that same project version. If a table already
exists from an earlier build of this release without that column, logging adds
it and assigns the current version to existing rows:

- `id`: generated integer primary key.
- Named input columns: `frequency_hz`, and `aperture_length_m` for far-field
  calls. Finite values are numeric; non-finite inputs are preserved as text
  `NaN`, `+Infinity`, or `-Infinity`.
- `output_is_null`: whether the caller supplied a null output pointer. Pointer
  addresses and pre-existing output contents are not stored.
- `return_value`: the numeric `amcore_status` returned by the function.
- `wavelength_m` or `distance_m`: the computed output, SQL NULL on failure.
- `executed_at_us`: UTC call-start time, in microseconds since the Unix epoch.
- `duration_ns`: nonnegative elapsed time from a monotonic clock, in nanoseconds.
  Clock resolution is platform-dependent. This excludes the call's own database
  write, but includes nested calls and their logging.

Logging is synchronous and uses a separate connection per record, allowing
concurrent calls and processes. SQLite lock waits are bounded to five seconds
per database operation. Filesystem, clock, or database errors are reported to
stderr with an `amcore operation logging:` prefix; they never change calculation
results, status codes, or output-on-error behavior. Failed records are not
retried or buffered. Logging uses allocation and adds I/O latency; do not enable
it for real-time workloads. Logs can contain sensitive inputs and grow without
automatic retention limits; manage access and cleanup accordingly.

### Telemetry

Enable telemetry independently of SQLite logging and supply the endpoint at
configuration time:

```powershell
cmake -S . -B build-telemetry `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=telemetry `
  -DAMCORE_ENABLE_TELEMETRY=ON `
  -DAMCORE_TELEMETRY_ENDPOINT=https://api.signalregistry.net `
  -DBUILD_TESTING=OFF
cmake --build build-telemetry --config Release
```

`https://api.signalregistry.net` is a configuration example, not a built-in
default or a verified telemetry service. Confirm that your server accepts JSON
POST requests at the configured path before using it.

Desktop builds require libwebsockets 4.3+ with HTTP client and TLS support
(`find_package(Libwebsockets CONFIG)`). All telemetry builds also require cJSON
(`find_package(cJSON CONFIG)`) for JSON serialization. The `telemetry` vcpkg
feature provides both desktop dependencies. Review their licenses and dependencies
before distribution. Installed CMake consumers discover these automatically;
shared runtime dependencies must be distributed separately.

Every calculation, including failures and nested calls, sends one synchronous
JSON POST with the library `version`, named inputs, output-pointer nullness,
status, successful output, UTC start time and duration. Errors go to stderr
with `amcore telemetry:` and never change calculation results or exit status.
There is no retry, queue,
disk storage, runtime toggle, or authentication configuration. Redirects are
not followed on desktop. Telemetry transmits potentially sensitive calculation
data; enable it only with appropriate consent and a trusted HTTPS endpoint.
libwebsockets may additionally emit its own platform diagnostics on stderr;
libamcore does not change the application's global libwebsockets log settings.

TLS certificate and hostname verification stay enabled. Desktop HTTPS uses
the libwebsockets TLS backend's default trust sources; optionally set
`-DAMCORE_TELEMETRY_CA_FILE=<absolute-path-to-ca-bundle.pem>`. This PEM bundle
must remain accessible on the deployment machine. On Windows, an explicit CA
bundle may be needed depending on the TLS backend's trust-store setup.
WebAssembly uses browser trust and CORS, not this CA file.

Embedded telemetry is deferred; leave it disabled for embedded Linux and
firmware. WebAssembly telemetry requires Emscripten JavaScript glue, pthread
worker-thread calls and cross-origin isolation; the default telemetry-disabled
standalone module is unchanged. See
[telemetry configuration and payload](docs/public/cmake-integration.md#telemetry)
and [WebAssembly telemetry](docs/public/webassembly.md#telemetry).

Telemetry integration tests require Node.js and an unused local port:

```powershell
cmake -S . -B build-telemetry-tests `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=telemetry `
  -DAMCORE_ENABLE_TELEMETRY=ON `
  -DAMCORE_TELEMETRY_ENDPOINT=http://127.0.0.1:18765/events `
  -DBUILD_TESTING=ON
cmake --build build-telemetry-tests --config Release
ctest --test-dir build-telemetry-tests -C Release --output-on-failure
```

CTest starts and stops a loopback-only mock server for each test. Configuration
rejects non-loopback endpoints when telemetry tests are enabled, preventing
test calculations from being transmitted to an external server.

Logging and telemetry share a generic internal operation record. Functions
declare named, typed input/output fields once; the backends iterate those
fields rather than recognizing particular function names. Existing calculation
tables and JSON payloads remain compatible. See
[instrumenting new functions](docs/public/cmake-integration.md#instrumenting-new-functions)
for the extension pattern and current field types.

## Testing

To run the tests, first configure and build the test target with CMake, then use CTest to execute them. Tests are opt-in so cross-compiling firmware does not also try to build or run a host test executable:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

## Command-line interface

The `amcore` command-line tool is built by default. Use `-DAMCORE_BUILD_CLI=OFF`
to omit it, for example in a firmware cross-build. Calculations are grouped
under their associated module, using lowercase command names and positional
arguments:

```sh
build/amcore help
build/amcore version
build/amcore utility wavelength 1e9
build/amcore farfield distance 1e9 1
build/amcore utility --help
build/amcore farfield distance --help
```

General, module, and calculation help display the compile-time operation
logging setting; enabled builds also show the database location template.
Help also reports the telemetry setting and, when enabled, its endpoint.
There is no runtime logging flag. The CLI automatically records calculations
through the library, including nested calls and out-of-range calculations.
Parsing errors, help, and version requests do not call the calculation API and
are not recorded. Successful stdout remains numeric-only; logging failures may
produce stderr diagnostics without changing the calculation exit status.
See [desktop operation logging](#desktop-operation-logging) for setup and schema.


## WebAssembly

With the Emscripten SDK installed and activated, configure and build the
standalone WebAssembly module:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DBUILD_TESTING=OFF
cmake --build build-wasm --target amcore_wasm
```

This produces `build-wasm/amcore.wasm`, exporting
`amcore_free_space_wavelength` and `amcore_far_field_distance` without a `main`
entry point. Both functions retain the C API: their final argument is a pointer
into WebAssembly linear memory where the result is written. The module is installed to the library
directory when `AMCORE_BUILD_WASM` is enabled.


## AI-assisted development

Repository-specific guidance for AI coding agents is in
[`docs/public/LLM_CONTEXT.md`](docs/public/LLM_CONTEXT.md), with the reusable development
workflow in [`.github/skills/amcore-development/SKILL.md`](.github/skills/amcore-development/SKILL.md).

## Embedded toolchain

For an embedded toolchain, pass its CMake toolchain file when configuring:

```sh
cmake -S . -B build-firmware \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/your/toolchain.cmake \
  -DBUILD_TESTING=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-firmware
```

The resulting static library can also be included directly in a firmware
project with `add_subdirectory()` and linked through the `amcore::amcore`
target. Its public header does not impose a C language standard on consumers.

To install the library, header, and CMake package:

```sh
cmake --install build --prefix install
```

Installed CMake consumers can use `find_package(amcore CONFIG REQUIRED)` and
link `amcore::amcore`.

## Public documentation

GitHub Actions builds and tests run only for release tags matching `v*`,
including tags automatically created when the CMake project version changes
on `main`. Ordinary `main` pushes check the version and build and deploy
documentation to GitHub Pages; other branch pushes and pull requests do not run
CI. Documentation also builds and deploys on release-tag pushes. Manual
documentation builds can select any branch or tag and do not deploy.

The public documentation is built with MkDocs and published to
[GitHub Pages](https://antennameasurement.github.io/libamcore/). To preview it
locally, install the documentation dependencies and start the development
server:

```sh
python -m pip install -r requirements-docs.txt
python -m mkdocs serve
```

Build the static site with:

```sh
python -m mkdocs build --strict
```

## vcpkg

The repository includes a vcpkg manifest, with SQLite declared in its
`operation-logging` feature and a pinned `builtin-baseline` for the built-in
registry. The baseline is required by Visual Studio's bundled vcpkg and
selects the dependency versions. In manifest mode, do not pass package names
such as `sqlite3` to `vcpkg install`. To install the feature's dependencies
manually, run this from the repository root:

```powershell
vcpkg install --x-feature=operation-logging
```

Alternatively, CMake can install the feature's dependencies as part of
configuration. To build with operation logging on Windows in PowerShell, set
`VCPKG_ROOT` to your vcpkg checkout and configure with both the vcpkg toolchain
and the feature enabled:

```powershell
cmake -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
cmake --build build-logging --config Release
```

The toolchain must be supplied on the build directory's first configuration.
If `build-logging` was previously configured without vcpkg, adding the
toolchain argument later may still leave SQLite undiscoverable. With CMake
3.24 or newer, reset the cached configuration and configure again:

```powershell
cmake --fresh -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
```

`--fresh` resets cached options, so include any other options your build needs.
With older CMake, use a new build directory instead. A working configuration
reports `Running vcpkg install` followed by `Found SQLite3`.

In manifest mode, the feature command or CMake configuration installs the
`sqlite3` dependency for the selected triplet and makes it available to CMake's
`find_package(SQLite3)`. When configuring with CMake, use the vcpkg toolchain
and enable the manifest feature as shown so the dependency is available to
that build.

Keep the baseline compatible with your vcpkg tool's scripts. When updating
dependency versions, update vcpkg first, then run `vcpkg x-update-baseline`
and review the manifest changes. Selecting a newer registry baseline with an
older bundled vcpkg can cause dependency builds to fail.

## Packaging

CPack packages the installed library, headers, CMake package configuration,
and CLI when enabled. Run `cpack` from the configured build directory to create
a ZIP archive on Windows or a tarball on other platforms.
Logging-enabled packages do not bundle SQLite; distribute its runtime library
separately if dynamically linked.

## References

[1] [IEEE Std 149-2021, IEEE Recommended Practice for Antenna Measurements](https://ieeexplore.ieee.org/document/9714428)
