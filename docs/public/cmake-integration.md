# CMake integration

## Add the project as a subdirectory

When `libamcore` is included in a larger CMake project with `add_subdirectory`,
link the exported build-tree alias:

```cmake
add_subdirectory(path/to/libamcore)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

The target supplies the public include directory, so application code can
include `<amcore/amcore.h>`.

## Use an installed package

After [installing](getting-started.md#install) the library, use its CMake
package:

```cmake
find_package(amcore CONFIG REQUIRED)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

## Configure options

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library. |
| `AMCORE_BUILD_CLI` | `ON` | Build and install the `amcore` command-line tool. |
| `BUILD_TESTING` | `OFF` | Build and register the tests. |
| `AMCORE_BUILD_WASM` | `OFF` | Build the standalone WebAssembly target; requires the Emscripten toolchain. |
| `AMCORE_ENABLE_OPERATION_LOGGING` | `OFF` | Record public calculation calls in SQLite on desktop Windows, Linux, or macOS. Not supported for WebAssembly or bare-metal targets; leave off for embedded Linux. |
| `AMCORE_ENABLE_TELEMETRY` | `OFF` | Send calculation events as synchronous JSON POSTs. Uses libwebsockets on desktop and Emscripten Fetch on WebAssembly. Leave off for embedded builds. |
| `AMCORE_TELEMETRY_ENDPOINT` | empty | Required absolute HTTP(S) endpoint when telemetry is enabled; no default destination. |
| `AMCORE_TELEMETRY_CA_FILE` | empty | Optional desktop PEM CA bundle path; otherwise use the TLS backend's default trust sources. Browser Fetch ignores this option. |

## Desktop operation logging

Enable `AMCORE_ENABLE_OPERATION_LOGGING` to store successful, failed, and nested
calls in `libamcore.db`, with one table per public calculation function.
SQLite 3.24+ development files must be available to CMake
(`find_package(SQLite3 3.24)`).
With vcpkg, use the repository's manifest feature so the dependency is installed
for the same triplet as the build. The manifest pins a `builtin-baseline` to
select registry dependency versions, including when using Visual Studio's
bundled vcpkg. CMake can install it during configuration;
in PowerShell, with `VCPKG_ROOT` set to the vcpkg checkout:

```powershell
cmake -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
cmake --build build-logging --config Release
```

The toolchain must be present on the first configuration of a build directory.
If that directory was previously configured without vcpkg and SQLite is not
found, reset the cached configuration with CMake 3.24 or newer:

```powershell
cmake --fresh -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
```

`--fresh` resets cached options; pass any additional build options again.
For older CMake versions, use a new build directory instead. Confirm that
configuration reports `Running vcpkg install` and `Found SQLite3`.

Alternatively, from the repository root, install the manifest feature
explicitly with `vcpkg install --x-feature=operation-logging`. In manifest
mode, `vcpkg install` does not accept individual package arguments such as
`sqlite3`. The feature or CMake configuration installs the dependency declared
by `operation-logging`; use the matching vcpkg toolchain when configuring.
Installed packages discover SQLite automatically for consuming applications.
Shared SQLite runtime dependencies must be distributed separately.

The database directory is `%LOCALAPPDATA%\libamcore` on Windows,
`$XDG_DATA_HOME/libamcore` (or `~/.local/share/libamcore`) on Linux, and
`~/Library/Application Support/libamcore` on macOS. Directories are created
automatically; relative or missing Linux XDG locations fall back to HOME.

Tables `amcore_free_space_wavelength` and `amcore_far_field_distance` contain an
integer `id`, named numeric inputs (`frequency_hz`, plus `aperture_length_m` for
far-field calls), `output_is_null`, the status code `return_value`, and the
numeric result (`wavelength_m` or `distance_m`, NULL on failure). Non-finite
inputs are stored as text `NaN`, `+Infinity`, or `-Infinity`.
`executed_at_us` is the UTC start time in Unix-epoch microseconds;
`duration_ns` is monotonic elapsed time in nanoseconds. Duration excludes the
call's own write but includes nested calls and their logging.

Logging allocates memory and writes synchronously, with up to five seconds of
SQLite lock waiting per database operation. Concurrent calls use independent
connections. Logging errors go to stderr without changing API results.
Records are not buffered or retried, and there is no automatic retention.
Protect and manage these files as potentially sensitive user data.
Disabled builds have no logging or SQLite dependency.
The CLI automatically uses the library's logging setting and reports it in
general, module, and calculation help. There is no runtime toggle; CLI parse
errors and help/version requests do not invoke calculation functions and are
not recorded. See [CLI logging behavior](cli.md#desktop-operation-logging).

## Telemetry

Telemetry and operation logging are independent and may be enabled together.
Disabled telemetry has no network dependency, allocation, clocks or I/O.
All enabled telemetry builds use cJSON for JSON serialization; provide its
`cJSON` config package and exported `cjson` target. cJSON handles text escaping,
finite floating-point formatting, and dynamically sized output. Exact 64-bit
integers use internally formatted integer literals rather than conversion to
cJSON's double-based number representation. Do not change cJSON's global
allocation hooks or the process locale while library calls are in progress.
Desktop telemetry requires libwebsockets 4.3+ configured with HTTP client and
TLS support. CMake uses its `Libwebsockets` config package and the exported
`websockets_shared` or `websockets` target. The vcpkg manifest feature is
`telemetry` and installs both cJSON and libwebsockets; enable both manifest features with
`"-DVCPKG_MANIFEST_FEATURES=telemetry;operation-logging"` when enabling both
instrumentation options.

Example (PowerShell, `VCPKG_ROOT` pointing to vcpkg):

```powershell
cmake -S . -B build-telemetry `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=telemetry `
  -DAMCORE_ENABLE_TELEMETRY=ON `
  -DAMCORE_TELEMETRY_ENDPOINT=https://api.signalregistry.net `
  -DBUILD_TESTING=OFF
cmake --build build-telemetry --config Release
```

This URL is an example supplied for configuration, not a built-in default or
a verified service. No requests are made during CMake configuration or build.
The endpoint must accept `POST` with `Content-Type: application/json` and return
a 2xx response. Use an absolute HTTP(S) URL with a DNS hostname or IPv4
address, optional port, and optional path/query. Credentials in URLs, IPv6
literals, fragments, spaces, backslashes, quotes and semicolons are unsupported.
There are no runtime endpoint overrides or built-in authentication headers.

One POST is sent for each completed public calculation, including failed and
nested calls. Example payload:

```json
{
  "function": "amcore_free_space_wavelength",
  "inputs": {"frequency_hz": 1000000000},
  "output_is_null": false,
  "return_value": 0,
  "output": {"wavelength_m": 0.299792458},
  "executed_at_us": 1792746000000000,
  "duration_ns": 100
}
```

Far-field inputs additionally contain `aperture_length_m`, and successful
output contains `distance_m`. Status values match `amcore_status`: 0 success,
1 invalid argument, 2 out of range. Failed outputs are JSON `null`; output
contents are never read on failure. Non-finite inputs are strings `"NaN"`,
`"+Infinity"`, or `"-Infinity"`. Pointer addresses, device/user identifiers
and pre-existing output contents are never sent.

Logging and telemetry share the UTC epoch-microsecond call start and monotonic
nanosecond duration. The duration is captured before either sink, excluding
the call's own database write and POST, but including all nested calls and
their instrumentation. SQLite is written first when both are enabled.
A database failure does not prevent telemetry, and a telemetry failure does
not change SQLite records or calculation behavior.

Delivery is synchronous and best-effort with no retries, buffering, retention,
or disk spooling. A five-second event-loop deadline bounds each desktop
network transaction after context creation; TLS setup, synchronous DNS and
platform scheduling can add latency. Each call uses its own context and
connection, with no shared event queue. Transport, timeout, serialization and
non-2xx errors are reported to stderr with `amcore telemetry:`. Results,
status codes and CLI exit codes are unchanged. No desktop redirects are
followed, preventing automatic forwarding of calculation data to another URL.
Delivery finishes on receipt of response headers; response bodies are not
retained or processed. libwebsockets may also emit native platform diagnostics
on stderr, especially on Windows. Its global log settings are not modified.

Desktop TLS verification is never disabled. libwebsockets uses its TLS
backend's default trust sources, or the optional
`AMCORE_TELEMETRY_CA_FILE` PEM bundle. This file path is compiled in and must
be valid at runtime; include it in deployment planning. Depending on the
Windows TLS backend, a CA bundle may be necessary. Distribute dynamic
libwebsockets and its runtime dependencies separately, and review their
licenses. Installed CMake packages propagate the dependency to consumers.

Only enable transmission with appropriate consent; payloads may contain
sensitive measurement data. Use HTTPS except for controlled loopback tests.
Do not enable telemetry for real-time workloads. Embedded support is deferred;
non-desktop systems are rejected and embedded Linux must keep it disabled.
See [WebAssembly](webassembly.md#telemetry) for worker-thread, browser security,
and JavaScript-runtime requirements.

### Telemetry tests

With `BUILD_TESTING=ON`, telemetry requires Node.js and a dedicated endpoint
such as `-DAMCORE_TELEMETRY_ENDPOINT=http://127.0.0.1:18765/events`.
CTest starts a local mock server per test, serializes access to the endpoint,
and shuts it down when the test finishes. Remote endpoints and WebAssembly
test builds are rejected to prevent accidental external transmission. The
production configuration can use any supported endpoint with
`BUILD_TESTING=OFF`. Tests cover payloads, successful/failed/nested calls,
numeric locale, non-2xx responses, redirects, connection loss and timeouts.

## Instrumenting new functions

Both sinks consume the same private `amcore_operation_event`; neither
recognizes calculation names or argument positions. A function declares its
named input/output descriptors once, begins timing, and routes every exit
through the return macro. For example, inside a C function with a double
parameter `input_hz` and a `double *result_m` output:

```c
AMCORE_OPERATION_FIELDS(inputs,
    {"input_hz", AMCORE_OPERATION_DOUBLE, &input_hz});
AMCORE_OPERATION_FIELDS(outputs,
    {"result_m", AMCORE_OPERATION_DOUBLE, result_m});
AMCORE_OPERATION_BEGIN(inputs, 1, outputs, 1);

/* Validate, calculate and assign *result_m only on success. */
AMCORE_OPERATION_RETURN(AMCORE_STATUS_SUCCESS);
```

Use `src/operation_log.h` for instrumentation. The name is captured with C99
`__func__`. `AMCORE_OPERATION_FIELDS` and the begin macro compile away when
both sinks are disabled, including their arguments; the return macro reduces
to an ordinary return. No instrumentation allocation or timing remains.
Functions with no inputs or outputs pass `NULL, 0` for that collection.
Multiple inputs and outputs are supported without backend edits.

Descriptors currently support:

| Type | `data` points to | SQLite | JSON |
| --- | --- | --- | --- |
| `AMCORE_OPERATION_DOUBLE` | `double` | REAL; non-finite values as text | number or non-finite string |
| `AMCORE_OPERATION_INT64` | `int64_t` | INTEGER (exact 64-bit value) | integer literal |
| `AMCORE_OPERATION_BOOL` | `int` | INTEGER 0/1 | boolean |
| `AMCORE_OPERATION_TEXT` | NUL-terminated UTF-8 text | TEXT | escaped string |

Null data is represented as SQL/JSON null, never an address. Output data is
not read unless status is success. On failure all output columns are NULL
and the JSON `output` is null. On success, JSON `output` is an object of named
values (or null for a single null output, preserving existing payloads);
an empty output collection is `{}`. `output_is_null` means any declared output
has a null data pointer; it is false when no outputs are declared.
JSON 64-bit integer literals are exact on the wire, but consumers using
JavaScript numbers must account for its safe-integer range.

Names must be ASCII identifiers and unique across inputs and outputs,
case-insensitively. Field names cannot use metadata column names (`id`,
`output_is_null`, `return_value`, `executed_at_us`, `duration_ns`).
Invalid descriptors produce sink diagnostics and skip recording without
changing calculation status. Keep descriptors and their referenced storage
alive until the return macro completes; retain entry-value copies if the
function mutates an input before returning. Output pointers refer to the
typed output storage, not the pointer variable itself. Do not register raw
addresses or secrets as fields.

SQLite generates a table and named columns from the descriptors, preserving
the existing wavelength/far-field schema and rows. Keep a function's field
schema stable across calls. Changing an already-deployed schema still needs
an explicit migration; the backend does not silently alter tables or discard
data. A mismatch is reported as a logging error. JSON and SQL construction
are dynamically sized rather than limited to the original two functions'
fixed buffers, subject to allocation, SQLite and HTTP transport limits.

C99 cannot reflect arbitrary C signatures automatically: each new function
must declare its fields and instrument its exits. Arrays and structured
results will need a shared field-type extension or explicit scalar summaries,
not per-function code inside the logging or telemetry backend.

## Embedded builds

For embedded or cross-compiled builds, provide the relevant CMake toolchain
file and commonly disable both tests and the CLI:

```sh
cmake -S . -B build-firmware \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DBUILD_TESTING=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DAMCORE_ENABLE_TELEMETRY=OFF \
  -DAMCORE_BUILD_CLI=OFF
```
