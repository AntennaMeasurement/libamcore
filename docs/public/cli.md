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

The executable, modules, and calculation commands accept `help`, `-h`, and
`--help`. The CLI is built by default; configure with
`-DAMCORE_BUILD_CLI=OFF` to omit it.

This branch preserves the calculation commands, help, and version output.
Operation logging, telemetry, and the `history` subcommand are not included.
