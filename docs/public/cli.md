# Command-line interface

The `amcore` executable provides help, version information, and the library's
two calculations:

```text
amcore help
amcore version
amcore utility wavelength <frequency_hz>
amcore farfield distance <frequency_hz> <aperture_length_m>
```

Logging-enabled builds additionally provide [call history](#call-history) with
tabular summaries and detailed record lookup.

Examples:

```sh
amcore utility wavelength 1e9
amcore farfield distance 1e9 1
```

Frequency is specified in hertz and aperture length in metres. Inputs must be
positive finite numbers; scientific notation is supported.

On success, a calculation prints only the numeric result in metres to standard
output. Invalid arguments and out-of-range results print a diagnostic to
standard error and return a nonzero exit status.

Running a module without a subcommand displays its help. The executable,
modules, and calculation commands accept `help`, `-h`, and `--help`.

The CLI is built by default. Configure with `-DAMCORE_BUILD_CLI=OFF` to omit
it.

## Desktop operation logging

Logging is a **compile-time setting**, off by default; there is no runtime
enable/disable flag. Build the CLI and library with
`-DAMCORE_ENABLE_OPERATION_LOGGING=ON` and SQLite development files available
to record calculations. General, module, and calculation help all display
`Operation logging: enabled` or `Operation logging: disabled`. Enabled help also
shows the platform's database location template; this does not open or create
the database. Version output remains `amcore <version>`.

For example, an enabled Windows build shows:

```text
Operation logging: enabled (SQLite, compile-time setting).
Database: %LOCALAPPDATA%\libamcore\libamcore.db
```

Each calculation that reaches the C API records its named inputs, status,
numeric output, UTC start time, and elapsed duration in a function-named table.
Far-field calculations also record their nested wavelength calls. Out-of-range
calculations reaching the library are recorded, but invalid command syntax,
numeric parsing errors, help, and version requests are not.

Successful calculation stdout remains a single numeric value, with no logging
messages. If recording fails, an `amcore operation logging:` diagnostic appears
on stderr without changing the calculation's result or exit status. A
successful calculation can therefore exit zero with a logging warning.

The feature is available only on desktop Windows, Linux, and macOS, not
WebAssembly or embedded targets. See
[desktop operation logging](cmake-integration.md#desktop-operation-logging)
for dependency setup, database locations, schema, timing, and retention limits.
On supported desktop systems, logging can be enabled together with telemetry;
the combined dependency setup is documented in the
[CMake guide](cmake-integration.md#enable-both-on-desktop).

### Call history

Only logging-enabled builds expose the `history` subcommand:

```sh
amcore history
amcore history -l 25
amcore history --limit 100
amcore history --function amcore_free_space_wavelength
amcore history --function amcore_free_space_wavelength --id 12
amcore history --help
```

It reads the local log database and lists calls across **all functions and
library versions**, including failed and nested calls. `-l` and `--limit`
set a global maximum row count, not a per-function or per-version count.
The default is **10**. Values must be positive decimal integers up to
`9223372036854775807`; `--limit=25` is also supported.

Output is an aligned ASCII table with borders and up to the requested number
of calls. The ID is displayed first:

```text
+----+------------------------------+---------+------------------+--------------+-------------+
| ID | Function                     | Version | executed_at_us   | return_value | duration_ns |
+----+------------------------------+---------+------------------+--------------+-------------+
| 12 | amcore_free_space_wavelength | 0.4.1   | 1700000000000024 | 0            | 1200        |
+----+------------------------------+---------+------------------+--------------+-------------+
```

| Column           | Meaning                                                               |
| ---------------- | --------------------------------------------------------------------- |
| `ID`             | Row identifier within the original function/version table             |
| `Function`       | Library function name                                                 |
| `Version`        | Recorded library version                                              |
| `executed_at_us` | Call start time in UTC epoch microseconds                             |
| `return_value`   | Numeric library status: 0 success, 1 invalid argument, 2 out of range |
| `duration_ns`    | Call duration in nanoseconds                                          |

Rows are sorted by start time descending, then table name ascending and row id
descending to break ties. Historical versioned tables lacking a `version`
column use the version suffix in the table name. For unversioned tables without
that column, the version is `unknown`. Function-specific input/output columns
are not included in this summary.

Use `--function <name>` to filter by the library function name, without a
version suffix. Combine `--function` with `--id <positive-id>` to show every
stored field for the matching call, including named inputs, output-pointer
nullness, status, outputs and timing. Each match has a function/version/ID
heading and a two-column Field/Value table. IDs are local to their original
tables: the same function and ID can match **multiple library versions**, and
all matching versions are displayed newest-first. Versionless legacy records
are also included. Details are not capped at 10; `--id` cannot be combined
with `--limit` and requires `--function`. If no call matches, the command produces
a diagnostic and a nonzero exit status. Function-only queries with no matches show an empty
summary table. The `--function=<name>` and `--id=<id>` forms are also supported.

SQL NULL is shown as `NULL`. Control characters, backslashes and `|` in stored
values are escaped as `\xHH` so they cannot break the table layout.

History opens the log database read-only; it does not create directories,
change the database, record calls, or send telemetry. An empty database prints
the header and borders with no data rows. Missing or unreadable databases,
invalid log schemas, corrupt databases and query failures produce stderr
diagnostics and a nonzero exit
status. Help (`help`, `-h`, `--help`) does not open the database. Disabled builds
neither advertise nor recognize `history`.

## Telemetry

Telemetry is a separate compile-time option, disabled by default.
`AMCORE_ENABLE_TELEMETRY=ON` requires `AMCORE_TELEMETRY_ENDPOINT`; desktop
builds require libwebsockets, while WebAssembly uses Emscripten Fetch. General,
module, and calculation help report
`Telemetry: enabled` or `Telemetry: disabled`; enabled help shows the endpoint.
There are no runtime telemetry flags.

Logging and telemetry can be enabled together on desktop Windows, Linux, and
macOS only. WebAssembly supports telemetry without logging; embedded builds
should keep both options disabled.

Calculation calls, including failures and nested calls, send JSON POST events.
Parsing failures, help and version do not invoke calculations or send events.
Delivery is synchronous. Telemetry failures produce `amcore telemetry:`
diagnostics on stderr without changing numeric stdout or calculation exit
status. A successful calculation can therefore exit zero with a network warning.
See [telemetry configuration](cmake-integration.md#telemetry) for dependencies,
payloads, TLS trust, privacy and timeout behavior.
