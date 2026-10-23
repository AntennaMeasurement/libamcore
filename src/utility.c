/* Last content update tag: v0.1.0 */
#include <amcore/utility.h>
#include "operation_log.h"

#include <math.h>
#include <stddef.h>

const double speed_of_light_m_per_s = 299792458.0;

amcore_status amcore_free_space_wavelength(double frequency_hz,
                                           double *wavelength_m)
{
    double wavelength;
    AMCORE_OPERATION_FIELDS(inputs,
        {"frequency_hz", AMCORE_OPERATION_DOUBLE, &frequency_hz});
    AMCORE_OPERATION_FIELDS(outputs,
        {"wavelength_m", AMCORE_OPERATION_DOUBLE, wavelength_m});
    AMCORE_OPERATION_BEGIN(inputs, 1, outputs, 1);

    if (wavelength_m == NULL || !isfinite(frequency_hz) ||
        frequency_hz <= 0.0) {
        AMCORE_OPERATION_RETURN(AMCORE_STATUS_INVALID_ARGUMENT);
    }

    wavelength = speed_of_light_m_per_s / frequency_hz;
    if (!isfinite(wavelength) || wavelength <= 0.0) {
        AMCORE_OPERATION_RETURN(AMCORE_STATUS_OUT_OF_RANGE);
    }

    *wavelength_m = wavelength;
    AMCORE_OPERATION_RETURN(AMCORE_STATUS_SUCCESS);
}
