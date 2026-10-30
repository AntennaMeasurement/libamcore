# Command-line interface

The `amcore` executable provides help, version information, and the library's
calculations. Subcommands correspond to the modules providing these calculations:

```text
Usage: amcore <subcommand> [options]
       amcore [--help | --version]

Subcommands:
  utility    Utility calculations
  farfield   Far-field calculations
  nearfield  Near-field calculations

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

```
Usage: amcore nearfield <subcommand> [arguments]

Subcommands:
  separation-distance  Calculate recommended near-field separation in metres
  planar               Planar near-field calculations
  cylindrical          Cylindrical near-field calculations
  spherical            Spherical near-field calculations
```

```
Usage: amcore nearfield planar <subcommand> [arguments]

Subcommands:
  sampling-spacing     Calculate sampling spacing in metres
  scan-length          Calculate required scan length in metres
  angle-of-view        Calculate angle of view in degrees
  sampling-parameters  Calculate centered sampling parameters
```

```
Usage: amcore nearfield cylindrical <subcommand> [arguments]

Subcommands:
  ideal-y-scan-length  Calculate ideal cylindrical Y scan length in metres
```

```
Usage: amcore nearfield spherical <subcommand> [arguments]

Subcommands:
  maximum-radius-extent  Calculate maximum radius extent in metres
```

Examples:

```sh
amcore utility wavelength 1e9
amcore farfield distance 1e9 1
amcore farfield line-of-sight 100
amcore farfield radio-horizon 100
amcore farfield path-loss 100 1 0 0
amcore nearfield planar sampling-spacing 1e9
amcore nearfield separation-distance 1e10
amcore nearfield planar scan-length 1 2 45
amcore nearfield planar angle-of-view 1 2 5
amcore nearfield planar sampling-parameters 299792458 1
amcore nearfield cylindrical ideal-y-scan-length 1 0.2 2 1 60 45
amcore nearfield spherical maximum-radius-extent 3 4 12
```

Frequency for `distance` is specified in hertz and aperture length in metres.
The horizon commands take a non-negative height in metres. `path-loss` takes
frequency in megahertz, range in kilometres, and transmit and receive gains in
decibels. Frequency and range must be positive; gains may be signed. All inputs
must be finite; scientific notation is supported.

Planar `sampling-spacing` accepts frequency in hertz. `scan-length` accepts
antenna cross-section and probe separation in metres followed by an angle in
degrees from 0 inclusive to 90 exclusive. `angle-of-view` accepts antenna
cross-section, probe separation, and scan length in metres.
`sampling-parameters` accepts frequency in hertz and minimum scan length in
metres; its one output line contains start position (m), stop position (m),
sample count, and spacing (mm). Spherical `maximum-radius-extent` accepts
signed x, y, and z maximum displacements in metres. Cylindrical
`ideal-y-scan-length` accepts antenna height, probe longest edge, probe radius, and
maximum radial extent in metres, followed by maximum azimuth and elevation in
degrees. Lengths must be non-negative, probe radius must exceed maximum radial
extent, azimuth must be in [0, 180], and elevation in [0, 90).
`separation-distance` accepts frequency in hertz and returns five free-space
wavelengths in metres.

On success, a calculation prints only the numeric result in the documented
unit to standard output. Invalid arguments and out-of-range results print a
diagnostic to standard error and return a nonzero exit status.

The executable, modules, and calculation commands accept `help`, `-h`, and
`--help`. Module help lists available subcommands and the help options.
The CLI is built by default; configure with
`-DAMCORE_BUILD_CLI=OFF` to omit it.
