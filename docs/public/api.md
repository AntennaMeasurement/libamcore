# C API

Include `<amcore/amcore.h>` to use the full public API, or include a module
header such as `<amcore/utility.h>` or `<amcore/farfield.h>`. Public
declarations use `AMCORE_API`; the API supports both C and C++ callers.

## Status values

Operations return an `amcore_status`:

| Value | Meaning |
| --- | --- |
| `AMCORE_STATUS_SUCCESS` | The calculation succeeded. |
| `AMCORE_STATUS_INVALID_ARGUMENT` | An argument is invalid, including a null output pointer. |
| `AMCORE_STATUS_OUT_OF_RANGE` | The calculation result is not representable as a positive finite `double`. |

Inputs must be finite and positive. Frequency is in hertz. Lengths, wavelength,
and distances are in metres. Output parameters are modified only on success.

## Optional operation logging

The public C API and status codes are unchanged when
`AMCORE_ENABLE_OPERATION_LOGGING=ON`. Every call to the two calculation
functions is recorded, including failed calls and the nested wavelength call
made by the far-field calculation after validating its aperture and output
pointer. A far-field call rejected before wavelength calculation produces only
its own record.

Recording stores named inputs, output-pointer nullness, return status, successful
numeric output, UTC start time, and monotonic duration in a per-function SQLite
table. Output values are SQL NULL on failure; the caller's output remains
untouched. Pointer addresses and previous output contents are not stored.
Logging failures emit a diagnostic to stderr but do not change API results.

This desktop-only option adds synchronous filesystem access, memory allocation,
and a SQLite dependency. Leave it off for embedded and WebAssembly targets.
See [desktop operation logging](cmake-integration.md#desktop-operation-logging)
for paths, exact columns and units, timing boundaries, and operational limits.

## Free-space wavelength

`amcore_free_space_wavelength` computes wavelength from frequency:

`lambda = c / f`

where `c` is the exported constant `speed_of_light_m_per_s` and `f` is the
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

## Minimum far-field distance

`amcore_far_field_distance` calculates the minimum distance using IEEE Std
149-2021, section 7.1.2:

$$R_{ff} = \max(5D, 1.6 \lambda, 2D^2/\lambda)$$

`D` is the largest antenna aperture length in metres. The wavelength is
calculated from the provided frequency.

```c
#include <amcore/amcore.h>

double distance_m;
amcore_status status =
    amcore_far_field_distance(1.0e9, 1.0, &distance_m);
if (status == AMCORE_STATUS_SUCCESS) {
    /* distance_m is the minimum far-field distance in metres */
}
```

Signature:

```c
amcore_status amcore_far_field_distance(
    double frequency_hz,
    double aperture_length_m,
    double *distance_m);
```
