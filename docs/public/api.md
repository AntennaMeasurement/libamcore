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
| `AMCORE_STATUS_OUT_OF_RANGE` | The calculation result is not representable as a positive finite `double`. |

Inputs must be finite and positive. Frequency is in hertz. Lengths, wavelength,
and distances are in metres. Output parameters are modified only on success.

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
