# Command-line interface

The `amcore` executable provides help, version information, and the library's
calculations. Subcommands correspond to the modules providing these calculations:

```text
Usage: amcore <subcommand> [options]
       amcore [--help | --version]

Subcommands:
  utility    Utility calculations
  farfield   Far-field calculations

Options:
  -h, --help     Show this help message
  -V, --version  Show the program version
```

```
Usage: amcore utility <subcommand> [arguments]

Subcommands:
  wavelength Calculate free-space wavelength in metres
```

```
Usage: amcore farfield <subcommand> [arguments]

Subcommands:
  distance  Calculate far-field distance in metres
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


