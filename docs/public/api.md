# C API

Include `<amcore/amcore.h>` to use the full public API, or include a module
header such as `<amcore/utility.h>`, `<amcore/farfield.h>`, or
`<amcore/nearfield.h>`. Public
declarations use `AMCORE_API`; the API supports both C and C++ callers.

## Status values

Operations return an `amcore_status`:

| Value | Meaning |
| --- | --- |
| `AMCORE_STATUS_SUCCESS` | The calculation succeeded. |
| `AMCORE_STATUS_INVALID_ARGUMENT` | An argument is invalid, including a null output pointer. |
| `AMCORE_STATUS_OUT_OF_RANGE` | The calculation result is not representable as a finite `double`. |

Inputs must be finite and within each function's documented domain. Frequency
is in hertz unless otherwise specified. Lengths and distances use the units
stated by each function. Output parameters are modified only on success.

## Utility Module

The utility module provides common calculations that are frequently used in antenna measurements, such as computing the free-space wavelength from a given frequency.

### Free-space wavelength

`amcore_free_space_wavelength_m` computes wavelength from frequency $\lambda = c / f$ where `c` is the exported constant `speed_of_light_m_per_s` and `f` is the
frequency in hertz.

```c
#include <amcore/amcore.h>

double wavelength_m;
amcore_status status = amcore_free_space_wavelength_m(1.0e9, &wavelength_m);
if (status == AMCORE_STATUS_SUCCESS) {
    /* wavelength_m is approximately 0.2998 metres */
}
```

Signature:

```c
amcore_status amcore_free_space_wavelength_m(
    double frequency_hz,
    double *wavelength_m);
```

## Far-field Module

The far-field module provides calculations related to the far-field region of an antenna, such as determining the minimum far-field distance.

### Far-field distance

`amcore_far_field_distance` implements IEEE Std 149-2021, section 7.1.2:

$$R_{ff} = max(5D, 1.6\lambda, 2D^2/\lambda)$$

where `D` is the largest aperture length and `lambda` is the free-space
wavelength.

```c
double distance_m;
amcore_status status = amcore_far_field_distance(1.0e9, 1.0, &distance_m);
```

Signature:

```c
amcore_status amcore_far_field_distance(
    double frequency_hz,
    double aperture_length_m,
    double *distance_m);
```

### Line-of-sight range

`amcore_far_field_line_of_sight_km` estimates the line-of-sight range using
$R = 3.57 * \sqrt{height_m}$`. Height is in metres; the result is in kilometres.
Height may be zero but must be finite and non-negative.
The approximation follows [line-of-sight propagation](https://en.wikipedia.org/wiki/Line-of-sight_propagation).

```c
double range_km;
amcore_status status = amcore_far_field_line_of_sight_km(100.0, &range_km);
```

Signature:

```c
amcore_status amcore_far_field_line_of_sight_km(
    double height_m,
    double *range_km);
```

### Radio horizon

`amcore_far_field_radio_horizon_km` estimates the radio horizon using
$R = 4.12 * \sqrt{height_m}$`. Height is in metres; the result is in kilometres.
Height may be zero but must be finite and non-negative.
The approximation follows [line-of-sight propagation](https://en.wikipedia.org/wiki/Line-of-sight_propagation).

```c
double range_km;
amcore_status status = amcore_far_field_radio_horizon_km(100.0, &range_km);
```

Signature:

```c
amcore_status amcore_far_field_radio_horizon_km(
    double height_m,
    double *range_km);
```

### Free-space path loss

`amcore_far_field_path_loss_db` computes path loss under far-field conditions:

$$L_P = 32.45 + 20\log_{10}(f_{\mathrm{MHz}}R_{\mathrm{km}}) - G_{\mathrm{tx}} - G_{\mathrm{rx}}$$

Frequency is in megahertz, range in kilometres, and both antenna gains and the
result are in decibels. Frequency and range must be finite and positive; gains
must be finite and may be signed.
This formula is described in IEEE Std 1720-2012, section B.2.2, and *Antenna
Theory and Design*, 3rd edition, page 43.

```c
double path_loss_db;
amcore_status status =
    amcore_far_field_path_loss_db(100.0, 1.0, 0.0, 0.0, &path_loss_db);
```

Signature:

```c
amcore_status amcore_far_field_path_loss_db(
    double frequency_mhz,
    double range_km,
    double transmit_gain_db,
    double receive_gain_db,
    double *path_loss_db);
```

## Near-field Module

The near-field module contains planar sampling and scan-geometry calculations,
cylindrical Y scan geometry, and a spherical near-field extent calculation.

### Recommended measurement distance

`amcore_near_field_measurement_distance_m` calculates the distance between the
probe and antenna aperture using $R = C\lambda$. A coefficient of zero selects
the default $C=5.0$, following IEEE Std 1720-2012, section 5.3.1.4. Any finite,
non-negative coefficient may be supplied.

```c
double separation_m;
amcore_status status =
    amcore_near_field_measurement_distance_m(10.0e9, 0.0, &separation_m);
```

Signature:

```c
amcore_status amcore_near_field_measurement_distance_m(
    double frequency_hz,
    double coeff,
    double *separation_m);
```

### Planar ideal step size

`amcore_near_field_planar_ideal_step_size_m` returns half the free-space
wavelength, $\Delta = \lambda/2$. This step size follows IEEE Std 149-2021,
section 12.5, and IEEE Std 1720-2012, section 5.2.5, equation 25.

```c
double step_size_m;
amcore_status status =
    amcore_near_field_planar_ideal_step_size_m(1.0e9, &step_size_m);
```

Signature:

```c
amcore_status amcore_near_field_planar_ideal_step_size_m(
    double frequency_hz,
    double *step_size_m);
```

### Planar ideal scan length and angle of view

`amcore_near_field_planar_ideal_scan_length_m` calculates
$L = 2d\tan(\theta)+a+p$, where AUT means Antenna Under Test, `a` is its
maximum edge length, `p` is the probe maximum edge length, `d` is the
probe-to-aperture measurement distance, and $\theta$ is the desired angle along
one side. Lengths are in metres and the angle is in degrees. The angle must be
in $[0,90)$; both edge lengths must be finite and non-negative.
The scan geometry follows IEEE Std 149-2021, section 12.5, equation 99, and
IEEE Std 1720-2012, section 5.3.1.6, equation 27.

```c
double scan_length_m;
amcore_status status = amcore_near_field_planar_ideal_scan_length_m(
    1.0, 0.5, 2.0, 45.0, &scan_length_m);
```

`amcore_near_field_planar_ideal_angle_of_view_deg` calculates the ideal angle
for a centered scan using
$\theta = \operatorname{atan2}(L-a-p,2d)$, where `a` is the AUT maximum edge
length, `p` is the probe maximum edge length, and `d` is the probe-to-aperture
measurement distance; the result is converted to degrees. A scan shorter than
the sum of the AUT and probe maximum edge lengths yields a negative angle, as
in the defining formula.

```c
double angle_of_view_deg;
amcore_status status = amcore_near_field_planar_ideal_angle_of_view_deg(
    1.0, 0.5, 2.0, 5.5, &angle_of_view_deg);
```

### Planar sampling parameters

`amcore_near_field_planar_sampling_parameters` rounds half-wavelength step
size down to whole millimetres, rounds the requested scan length up to whole
millimetres, and extends the centered half-scan to a whole multiple of the
step size. It returns the integer step size in millimetres, calculated scan
length, negative and positive endpoints in metres, and sample count including
both endpoints. It returns `AMCORE_STATUS_OUT_OF_RANGE` if the step size
rounds down to zero millimetres or the values cannot be represented.

```c
size_t step_size_mm;
double scan_length_m;
double scan_start_m;
double scan_stop_m;
size_t sample_count;
amcore_status status = amcore_near_field_planar_sampling_parameters(
    299792458.0, 1.0, &step_size_mm, &scan_length_m, &scan_start_m,
    &scan_stop_m, &sample_count);
```

### Ideal cylindrical Y step size

`amcore_near_field_cylindrical_ideal_y_step_size_m` returns half the free-space
wavelength without rounding, expressed in metres:

$$\Delta_Y = \frac{\lambda}{2}$$

The step size depends only on frequency, so the unused `scan_size` argument
from the Python calculation is omitted. Frequency must be finite and positive.
Submillimetre steps are supported. The API returns `AMCORE_STATUS_OUT_OF_RANGE`
if the wavelength or step size is not representable as a positive finite
`double`. The output is unchanged on error.

```c
double step_size_m;
amcore_status status =
    amcore_near_field_cylindrical_ideal_y_step_size_m(1.0e9, &step_size_m);
/* On success, step_size_m is 0.149896229 metres. */
```

Signature:

```c
amcore_status amcore_near_field_cylindrical_ideal_y_step_size_m(
    double frequency_hz,
    double *step_size_m);
```

### Ideal cylindrical Y scan length

`amcore_near_field_cylindrical_ideal_y_scan_length_m` calculates the ideal
cylindrical Y scan length using the NSI 2000 (Near-field Edition), Version 4,
Software Operating Manual geometry:

$$L_{\mathrm{y}} = H_{\mathrm{AUT}} + H_{\mathrm{probe}} + 2(R_{\mathrm{probe}} - MRE\cos(\mathrm{Az}_{max}))\tan(\mathrm{El}_{max})$$

This is the ideal geometric scan length described by the manual. Here
$H_{\mathrm{AUT}}$ is the antenna height, $H_{\mathrm{probe}}$ is the
probe's longest edge, $R_{\mathrm{probe}}$ is the probe radius, and $MRE$ is
the maximum radial extent, all in metres. Length inputs must be finite and
non-negative, and the probe radius must be greater than the maximum radial
extent.

Azimuth must be finite and in [0, 180] degrees; elevation must be finite and
in [0, 90) degrees. Elevations above 60 degrees are accepted without a warning.
The output pointer is required and the output remains unchanged on error.

```c
double scan_length_m;
amcore_status status = amcore_near_field_cylindrical_ideal_y_scan_length_m(
    1.0, 0.2, 2.0, 1.0, 60.0, 45.0,
    &scan_length_m);
/* On success: scan_length_m is approximately 4.2. */
```

Signature:

```c
amcore_status amcore_near_field_cylindrical_ideal_y_scan_length_m(
    double antenna_height_m,
    double probe_longest_edge_m,
    double probe_radius_m,
    double maximum_radial_extent_m,
    double azimuth_max_deg,
    double elevation_max_deg,
    double *scan_length_m);
```

The NSI2000 reference case with antenna height 0.2 m, probe longest edge
0.247 m, probe radius 2.5 m, maximum radial extent 0.6 m, azimuth 180 degrees,
and elevation 60 degrees produces **11.186 m** when rounded to three decimal
places.

### Ideal cylindrical elevation angle of view

`amcore_near_field_cylindrical_ideal_elevation_angle_of_view_deg` inverts the
cylindrical Y scan-length geometry for a centered scan:

$$\theta_{\mathrm{el}} = \operatorname{atan2}(L_{\mathrm{y}} - H_{\mathrm{AUT}} - H_{\mathrm{probe}}, 2(R_{\mathrm{probe}} - MRE\cos(\mathrm{Az}_{max})))$$

The result is converted to degrees. Length inputs must be finite and
non-negative except that scan length must be positive. The probe radius must be
greater than the maximum radial extent, and azimuth must be in [0, 180]
degrees. The output remains unchanged on error.

```c
double angle_of_view_deg;
amcore_status status =
    amcore_near_field_cylindrical_ideal_elevation_angle_of_view_deg(
        1.0, 0.2, 2.0, 1.0, 60.0, 5.2, &angle_of_view_deg);
/* On success: angle_of_view_deg is approximately 53.130 degrees. */
```

Signature:

```c
amcore_status amcore_near_field_cylindrical_ideal_elevation_angle_of_view_deg(
    double aut_height_m,
    double probe_longest_edge_m,
    double probe_radius_m,
    double maximum_radial_extent_m,
    double azimuth_max_deg,
    double scan_length_m,
    double *angle_of_view_deg);
```

### Spherical maximum radius extent

`amcore_near_field_spherical_maximum_radius_extent_m` calculates the
Euclidean norm of the maximum x, y, and z displacements. Displacements may be
signed; the result is in metres.

```c
double extent_m;
amcore_status status =
    amcore_near_field_spherical_maximum_radius_extent_m(
        3.0, 4.0, 12.0, &extent_m);
```
