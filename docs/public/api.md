# C API

Include `<amcore/amcore.h>` to use the full public API, or include a module
header such as `<amcore/utility.h>` or `<amcore/farfield.h>`. Public
declarations use `AMCORE_API`; the API supports both C and C++ callers.

## Machine-readable manifest

[api.json](api.json) describes the library version, public functions, input
schemas, output names and units, status codes, and examples. 

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

`amcore_free_space_wavelength` computes wavelength from frequency $\lambda = c / f$ where `c` is the exported constant `speed_of_light_m_per_s` and `f` is the
frequency in hertz.

```c
#include <amcore/amcore.h>

double wavelength_m;
amcore_status status = amcore_free_space_wavelength(1.0e9, &wavelength_m);
if (status == AMCORE_STATUS_SUCCESS) {
    /* wavelength_m is approximately 0.2998 metres */
}
```

Signature:

```c
amcore_status amcore_free_space_wavelength(
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
