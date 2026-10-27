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

static amcore_status calculate_horizon(double height_m, double coefficient,
                                       double *range_km)
{
    double range;

    if (range_km == NULL || !isfinite(height_m) || height_m < 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    range = coefficient * sqrt(height_m);
    if (!isfinite(range)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *range_km = range;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_far_field_line_of_sight_km(double height_m,
                                                double *range_km)
{
    return calculate_horizon(height_m, 3.57, range_km);
}

amcore_status amcore_far_field_radio_horizon_km(double height_m,
                                                double *range_km)
{
    return calculate_horizon(height_m, 4.12, range_km);
}

amcore_status amcore_far_field_path_loss_db(double frequency_mhz,
                                            double range_km,
                                            double transmit_gain_db,
                                            double receive_gain_db,
                                            double *path_loss_db)
{
    double gain_sum;
    double path_loss;

    if (path_loss_db == NULL || !isfinite(frequency_mhz) ||
        frequency_mhz <= 0.0 || !isfinite(range_km) || range_km <= 0.0 ||
        !isfinite(transmit_gain_db) || !isfinite(receive_gain_db)) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    /* Logarithms avoid overflow in frequency * range; combine gains first
     * so large gains of opposite sign do not erase the path-loss term. */
    gain_sum = transmit_gain_db + receive_gain_db;
    path_loss =
        32.45 + 20.0 * (log10(frequency_mhz) + log10(range_km)) - gain_sum;
    if (!isfinite(gain_sum) || !isfinite(path_loss)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *path_loss_db = path_loss;
    return AMCORE_STATUS_SUCCESS;
}
