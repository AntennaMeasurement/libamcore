# libamcore

`libamcore` is a portable C library of antenna-measurement calculations.

**Library version:** {{ library_version }}

## Supported calculations

| Module | Function | Result |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength` | Free-space wavelength in metres |
| `farfield` | `amcore_far_field_distance` | Minimum far-field distance in metres |
| `farfield` | `amcore_far_field_line_of_sight_km` | Line-of-sight range in kilometres |
| `farfield` | `amcore_far_field_radio_horizon_km` | Radio horizon in kilometres |
| `farfield` | `amcore_far_field_path_loss_db` | Free-space path loss in decibels |
| `nearfield` | `amcore_near_field_separation_distance_m` | Recommended near-field separation in metres |
| `nearfield` | `amcore_near_field_planar_sampling_spacing_m` | Planar sampling spacing in metres |
| `nearfield` | `amcore_near_field_planar_scan_length_m` | Planar scan length in metres |
| `nearfield` | `amcore_near_field_planar_angle_of_view_deg` | Planar angle of view in degrees |
| `nearfield` | `amcore_near_field_planar_sampling_parameters` | Centered planar sampling parameters |
| `nearfield` | `amcore_near_field_spherical_maximum_radius_extent_m` | Spherical maximum radius extent in metres |

All calculations are referenced from the relevant standards and literature. See the [References](#references) section for details.

## Start here

- [Build and install](getting-started.md)
- [C API reference](api.md)
- [Command-line interface](cli.md)
- [CMake integration](cmake-integration.md)
- [WebAssembly](webassembly.md)

Only the calculations listed above are currently implemented. Chamber design,
device control, pattern processing, and post-processing are not part of the
current public API.

See the [source repository](https://github.com/AntennaMeasurement/libamcore)
for releases, issues, and contribution information.

## References

[1] [IEEE Std 149-2021](https://standards.ieee.org/standard/149-2021.html) - Standard for Antenna Measurements
