#include <amcore/amcore.h>

#include <math.h>
#include <stddef.h>

amcore_status amcore_free_space_wavelength(double frequency_hz,
                                           double *wavelength_m)
{
    const double speed_of_light_m_per_s = 299792458.0;
    double wavelength;

    if (wavelength_m == NULL || !isfinite(frequency_hz) ||
        frequency_hz <= 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    wavelength = speed_of_light_m_per_s / frequency_hz;
    if (!isfinite(wavelength) || wavelength <= 0.0) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *wavelength_m = wavelength;
    return AMCORE_STATUS_SUCCESS;
}
