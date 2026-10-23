# Command-line interface

The `amcore` executable provides help, version information, and the library's
two calculations:

```text
amcore help
amcore version
amcore utility wavelength <frequency_hz>
amcore farfield distance <frequency_hz> <aperture_length_m>
```

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
