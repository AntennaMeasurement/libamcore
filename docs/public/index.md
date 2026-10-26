# libamcore

`libamcore` is a portable C library of antenna-measurement calculations. This
branch includes the calculation API and CLI; operation logging, call history,
and telemetry are not included.

**Library version:** {{ library_version }}

## Supported calculations

| Module | Function | Result |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength` | Free-space wavelength in metres |
| `farfield` | `amcore_far_field_distance` | Minimum far-field distance in metres |

All calculations are referenced from the relevant standards and literature. See the [References](#references) section for details.

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

## References

[1] [IEEE Std 149-2021](https://standards.ieee.org/standard/149-2021.html) - Standard for Antenna Measurements
