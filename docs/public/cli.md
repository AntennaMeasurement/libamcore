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
  distance       Calculate far-field distance in metres
  line-of-sight  Calculate line-of-sight range in kilometres
  radio-horizon  Calculate radio horizon in kilometres
  path-loss      Calculate free-space path loss in decibels
```

Examples:

```sh
amcore utility wavelength 1e9
amcore farfield distance 1e9 1
amcore farfield line-of-sight 100
amcore farfield radio-horizon 100
amcore farfield path-loss 100 1 0 0
```

Frequency for `distance` is specified in hertz and aperture length in metres.
The horizon commands take a non-negative height in metres. `path-loss` takes
frequency in megahertz, range in kilometres, and transmit and receive gains in
decibels. Frequency and range must be positive; gains may be signed. All inputs
must be finite; scientific notation is supported.

On success, a calculation prints only the numeric result in the documented
unit to standard output. Invalid arguments and out-of-range results print a
diagnostic to standard error and return a nonzero exit status.

The executable, modules, and calculation commands accept `help`, `-h`, and
`--help`. The CLI is built by default; configure with
`-DAMCORE_BUILD_CLI=OFF` to omit it.

