/* Last content update tag: v0.1.0 */
#include <amcore/farfield.h>

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

    if (distance_m == NULL || !isfinite(aperture_length_m) ||
        aperture_length_m <= 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    status = amcore_free_space_wavelength(frequency_hz, &wavelength);
    if (status != AMCORE_STATUS_SUCCESS) {
        return status;
    }

    distance = 5.0 * aperture_length_m;
    wavelength_limit = 1.6 * wavelength;
    /* Divide before multiplying to avoid squaring overflow or underflow. */
    aperture_limit = (aperture_length_m / wavelength) * aperture_length_m * 2.0;
    if (!isfinite(distance) || !isfinite(wavelength_limit) ||
        !isfinite(aperture_limit)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    if (wavelength_limit > distance) {
        distance = wavelength_limit;
    }
    if (aperture_limit > distance) {
        distance = aperture_limit;
    }

    *distance_m = distance;
    return AMCORE_STATUS_SUCCESS;
}
