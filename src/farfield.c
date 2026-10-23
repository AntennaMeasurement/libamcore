/* Last content update tag: v0.1.0 */
#include <amcore/farfield.h>
#include "operation_log.h"

#include <math.h>
#include <stddef.h>

amcore_status amcore_far_field_distance(double frequency_hz,
                                        double aperture_length_m,
                                        double *distance_m)
{
    double wavelength;
    double distance;
    double wavelength_limit;
    double aperture_limit;
    amcore_status status;
    AMCORE_OPERATION_BEGIN();

    if (distance_m == NULL || !isfinite(aperture_length_m) ||
        aperture_length_m <= 0.0) {
        AMCORE_OPERATION_RETURN("amcore_far_field_distance", frequency_hz,
                                aperture_length_m, distance_m,
                                AMCORE_STATUS_INVALID_ARGUMENT);
    }

    status = amcore_free_space_wavelength(frequency_hz, &wavelength);
    if (status != AMCORE_STATUS_SUCCESS) {
        AMCORE_OPERATION_RETURN("amcore_far_field_distance", frequency_hz,
                                aperture_length_m, distance_m, status);
    }

    distance = 5.0 * aperture_length_m;
    wavelength_limit = 1.6 * wavelength;
    /* Divide before multiplying to avoid squaring overflow or underflow. */
    aperture_limit = (aperture_length_m / wavelength) * aperture_length_m * 2.0;
    if (!isfinite(distance) || !isfinite(wavelength_limit) ||
        !isfinite(aperture_limit)) {
        AMCORE_OPERATION_RETURN("amcore_far_field_distance", frequency_hz,
                                aperture_length_m, distance_m,
                                AMCORE_STATUS_OUT_OF_RANGE);
    }

    if (wavelength_limit > distance) {
        distance = wavelength_limit;
    }
    if (aperture_limit > distance) {
        distance = aperture_limit;
    }

    *distance_m = distance;
    AMCORE_OPERATION_RETURN("amcore_far_field_distance", frequency_hz,
                            aperture_length_m, distance_m, AMCORE_STATUS_SUCCESS);
}
