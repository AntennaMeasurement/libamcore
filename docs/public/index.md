# libamcore

`libamcore` is a portable C library of antenna-measurement calculations. This
branch includes the calculation API and CLI; operation logging, call history,
and telemetry are not included.

## Supported calculations

| Module | Function | Result |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength` | Free-space wavelength in metres |
| `farfield` | `amcore_far_field_distance` | Minimum far-field distance in metres |

Far-field distance follows IEEE Std 149-2021, section 7.1.2:
`R_ff = max(5D, 1.6 lambda, 2D^2/lambda)`, where `D` is the largest antenna
aperture length and `lambda` is the free-space wavelength.

## Start here

- [Build and install](getting-started.md)
- [C API reference](api.md)
- [Command-line interface](cli.md)
- [CMake integration](cmake-integration.md)
- [WebAssembly](webassembly.md)

Only the calculations listed above are currently implemented. Near-field
calculations, chamber design, device control, pattern processing, and
post-processing are not part of the current public API.

See the [source repository](https://github.com/AntennaMeasurement/libamcore)
for releases, issues, and contribution information.
