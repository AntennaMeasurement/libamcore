# libamcore

`libamcore` is a portable C library of antenna-measurement calculations.

**Library version:** {{ library_version }}

## Supported calculations

| Module | Function | Result |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength_m` | Free-space wavelength in metres |
| `farfield` | `amcore_far_field_distance` | Minimum far-field distance in metres |
| `farfield` | `amcore_far_field_line_of_sight_km` | Line-of-sight range in kilometres |
| `farfield` | `amcore_far_field_radio_horizon_km` | Radio horizon in kilometres |
| `farfield` | `amcore_far_field_path_loss_db` | Free-space path loss in decibels |
| `nearfield` | `amcore_near_field_measurement_distance_m` | Recommended probe-to-aperture measurement distance in metres |
| `nearfield` | `amcore_near_field_planar_ideal_step_size_m` | Ideal planar step size in metres |
| `nearfield` | `amcore_near_field_planar_ideal_scan_length_m` | Ideal planar scan length in metres |
| `nearfield` | `amcore_near_field_planar_ideal_angle_of_view_deg` | Ideal planar angle of view in degrees |
| `nearfield` | `amcore_near_field_planar_sampling_parameters` | Centered planar sampling parameters |
| `nearfield` | `amcore_near_field_cylindrical_ideal_y_scan_length_m` | Ideal cylindrical Y scan length in metres |
| `nearfield` | `amcore_near_field_cylindrical_ideal_y_step_size_m` | Ideal cylindrical Y step size (half wavelength) in metres |
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

[1] [IEEE Std 149-2021](https://standards.ieee.org/standard/149-2021.html) - IEEE Recommended Practice for Antenna Measurements

[2] [Line-of-sight propagation](https://en.wikipedia.org/wiki/Line-of-sight_propagation) - Approximate line-of-sight range and radio horizon formulas

[3] [IEEE Std 1720-2012](https://standards.ieee.org/standard/1720-2012.html) - IEEE Recommended Practice for Near-Field Antenna Measurements

[4] Warren L. Stutzman and Gary A. Thiele, *Antenna Theory and Design*, 3rd edition

[5] NSI 2000 (Near-field Edition), Version 4, Software Operating Manual
