/* Last content update tag: v0.1.0 */
#include <amcore/nearfield.h>

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

amcore_status amcore_near_field_measurement_distance_m(
    double frequency_hz,
    double coeff,
    double *separation_m)
{
    double wavelength;
    double separation;
    double coefficient;
    amcore_status status;

    if (separation_m == NULL || !isfinite(coeff) || coeff < 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    coefficient = coeff == 0.0 ? 5.0 : coeff;
    status = amcore_free_space_wavelength_m(frequency_hz, &wavelength);
    if (status != AMCORE_STATUS_SUCCESS) {
        return status;
    }

    separation = coefficient * wavelength;
    if (!isfinite(separation) || separation <= 0.0) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *separation_m = separation;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_planar_ideal_step_size_m(
    double frequency_hz,
    double *step_size_m)
{
    double wavelength;
    double step_size;
    amcore_status status;

    if (step_size_m == NULL) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    status = amcore_free_space_wavelength_m(frequency_hz, &wavelength);
    if (status != AMCORE_STATUS_SUCCESS) {
        return status;
    }

    step_size = 0.5 * wavelength;
    if (!isfinite(step_size) || step_size <= 0.0) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *step_size_m = step_size;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_planar_ideal_scan_length_m(
    double aut_longest_edge_length_m,
    double probe_longest_edge_length_m,
    double measurement_distance_m,
    double angle_of_view_deg,
    double *scan_length_m)
{
    double pi;
    double angle_rad;
    double scan_length;

    if (scan_length_m == NULL || !isfinite(aut_longest_edge_length_m) ||
        aut_longest_edge_length_m < 0.0 ||
        !isfinite(probe_longest_edge_length_m) ||
        probe_longest_edge_length_m < 0.0 ||
        !isfinite(measurement_distance_m) || measurement_distance_m <= 0.0 ||
        !isfinite(angle_of_view_deg) ||
        angle_of_view_deg < 0.0 || angle_of_view_deg >= 90.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    pi = acos(-1.0);
    angle_rad = angle_of_view_deg * (pi / 180.0);
    scan_length = measurement_distance_m * (2.0 * tan(angle_rad));
    scan_length += aut_longest_edge_length_m;
    scan_length += probe_longest_edge_length_m;
    if (!isfinite(scan_length)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *scan_length_m = scan_length;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_planar_ideal_angle_of_view_deg(
    double aut_longest_edge_length_m,
    double probe_longest_edge_length_m,
    double measurement_distance_m,
    double scan_length_m,
    double *angle_of_view_deg)
{
    double angle;

    if (angle_of_view_deg == NULL ||
        !isfinite(aut_longest_edge_length_m) ||
        aut_longest_edge_length_m < 0.0 ||
        !isfinite(probe_longest_edge_length_m) ||
        probe_longest_edge_length_m < 0.0 ||
        !isfinite(measurement_distance_m) || measurement_distance_m <= 0.0 ||
        !isfinite(scan_length_m) || scan_length_m < 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    if (measurement_distance_m <= DBL_MAX / 2.0) {
        angle = atan2(scan_length_m - aut_longest_edge_length_m -
                          probe_longest_edge_length_m,
                      2.0 * measurement_distance_m);
    } else {
        angle = atan2(0.5 * scan_length_m -
                          0.5 * aut_longest_edge_length_m -
                          0.5 * probe_longest_edge_length_m,
                      measurement_distance_m);
    }
    angle *= 180.0 / acos(-1.0);
    if (!isfinite(angle)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *angle_of_view_deg = angle;
    return AMCORE_STATUS_SUCCESS;
}

static int double_to_size_t(double value, size_t *result)
{
    if (!isfinite(value) || value < 0.0 || value >= (double)SIZE_MAX) {
        return 0;
    }

    *result = (size_t)value;
    return 1;
}

amcore_status amcore_near_field_planar_sampling_parameters(
    double frequency_hz,
    double minimum_scan_length_m,
    size_t *step_size_mm,
    double *scan_length_m,
    double *scan_start_m,
    double *scan_stop_m,
    size_t *sample_count)
{
    double spacing_m;
    double spacing_mm_value;
    double scan_length_mm_value;
    size_t spacing_mm;
    size_t scan_length_mm;
    size_t half_scan_mm;
    size_t remainder;
    size_t adjustment;
    size_t interval_count;
    size_t count;
    double scan_length;
    double start;
    double stop;
    amcore_status status;

    if (step_size_mm == NULL || scan_length_m == NULL ||
        scan_start_m == NULL || scan_stop_m == NULL ||
        sample_count == NULL ||
        !isfinite(minimum_scan_length_m) || minimum_scan_length_m < 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    status = amcore_near_field_planar_ideal_step_size_m(frequency_hz,
                                                         &spacing_m);
    if (status != AMCORE_STATUS_SUCCESS) {
        return status;
    }

    spacing_mm_value = floor(spacing_m * 1000.0);
    scan_length_mm_value = ceil(minimum_scan_length_m * 1000.0);
    if (!double_to_size_t(spacing_mm_value, &spacing_mm) ||
        spacing_mm == 0 ||
        !double_to_size_t(scan_length_mm_value, &scan_length_mm)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    half_scan_mm = scan_length_mm / 2 + scan_length_mm % 2;
    remainder = half_scan_mm % spacing_mm;
    adjustment = remainder == 0 ? 0 : spacing_mm - remainder;
    if (half_scan_mm > SIZE_MAX - adjustment) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }
    half_scan_mm += adjustment;

    interval_count = half_scan_mm / spacing_mm;
    if (interval_count > (SIZE_MAX - 1) / 2) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }
    count = 2 * interval_count + 1;
    start = -((double)half_scan_mm / 1000.0);
    stop = (double)half_scan_mm / 1000.0;
    scan_length = 2 * stop;
    if (!isfinite(start) || !isfinite(stop) || !isfinite(scan_length)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *step_size_mm = spacing_mm;
    *scan_length_m = scan_length;
    *scan_start_m = start;
    *scan_stop_m = stop;
    *sample_count = count;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_cylindrical_ideal_y_step_size_m(
    double frequency_hz,
    double *step_size_m)
{
    return amcore_near_field_planar_ideal_step_size_m(frequency_hz,
                                                      step_size_m);
}

amcore_status amcore_near_field_cylindrical_ideal_y_scan_length_m(
    double antenna_height_m,
    double probe_longest_edge_m,
    double probe_radius_m,
    double maximum_radial_extent_m,
    double azimuth_max_deg,
    double elevation_max_deg,
    double *scan_length_m)
{
    double radians_per_degree;
    double azimuth_cos;
    double elevation_scale;
    double extension;
    double scan_length;

    if (scan_length_m == NULL ||
        !isfinite(antenna_height_m) || antenna_height_m < 0.0 ||
        !isfinite(probe_longest_edge_m) || probe_longest_edge_m < 0.0 ||
        !isfinite(probe_radius_m) || !isfinite(maximum_radial_extent_m) ||
        maximum_radial_extent_m < 0.0 ||
        probe_radius_m <= maximum_radial_extent_m ||
        !isfinite(azimuth_max_deg) || azimuth_max_deg < 0.0 ||
        azimuth_max_deg > 180.0 || !isfinite(elevation_max_deg) ||
        elevation_max_deg < 0.0 || elevation_max_deg >= 90.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    radians_per_degree = acos(-1.0) / 180.0;
    azimuth_cos = cos(azimuth_max_deg * radians_per_degree);
    elevation_scale = 2.0 * tan(elevation_max_deg * radians_per_degree);
    if (azimuth_cos >= 0.0) {
        extension = (probe_radius_m -
                     maximum_radial_extent_m * azimuth_cos) * elevation_scale;
    } else {
        /* Scale before adding to avoid overflow in Z - R cos(Az). */
        extension = probe_radius_m * elevation_scale +
                    maximum_radial_extent_m * (-azimuth_cos * elevation_scale);
    }
    scan_length = antenna_height_m + probe_longest_edge_m + extension;
    if (!isfinite(scan_length)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *scan_length_m = scan_length;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_cylindrical_ideal_elevation_angle_of_view_deg(
    double aut_height_m,
    double probe_longest_edge_m,
    double probe_radius_m,
    double maximum_radial_extent_m,
    double azimuth_max_deg,
    double scan_length_m,
    double *angle_of_view_deg)
{
    /* Invert the cylindrical Y scan-length formula. */
    if (angle_of_view_deg == NULL ||
        !isfinite(aut_height_m) || aut_height_m < 0.0 ||
        !isfinite(probe_longest_edge_m) || probe_longest_edge_m < 0.0 ||
        !isfinite(probe_radius_m) || !isfinite(maximum_radial_extent_m) ||
        maximum_radial_extent_m < 0.0 ||
        probe_radius_m <= maximum_radial_extent_m ||
        !isfinite(azimuth_max_deg) || azimuth_max_deg < 0.0 ||
        azimuth_max_deg > 180.0 ||
        !isfinite(scan_length_m) || scan_length_m <= 0.0) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    double radians_per_degree = acos(-1.0) / 180.0;

    double numerator = scan_length_m - aut_height_m - probe_longest_edge_m;
    double denominator =
        2.0 * (probe_radius_m -
               maximum_radial_extent_m *
                   cos(azimuth_max_deg * radians_per_degree));
    double angle_of_view_deg_local =
        atan2(numerator, denominator) * 180.0 / acos(-1.0);

    if (!isfinite(angle_of_view_deg_local)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *angle_of_view_deg = angle_of_view_deg_local;
    return AMCORE_STATUS_SUCCESS;
}

amcore_status amcore_near_field_cylindrical_elevation_sampling_parameters(
    double frequency_hz,
    double minimum_scan_length_m,
    size_t *step_size_mm,
    double *scan_length_m,
    double *scan_start_m,
    double *scan_stop_m,
    size_t *sample_count)
{
    return amcore_near_field_planar_sampling_parameters(
        frequency_hz,
        minimum_scan_length_m,
        step_size_mm,
        scan_length_m,
        scan_start_m,
        scan_stop_m,
        sample_count);
}

amcore_status amcore_near_field_spherical_maximum_radius_extent_m(
    double displacement_x_m,
    double displacement_y_m,
    double displacement_z_m,
    double *extent_m)
{
    double extent;

    if (extent_m == NULL || !isfinite(displacement_x_m) ||
        !isfinite(displacement_y_m) || !isfinite(displacement_z_m)) {
        return AMCORE_STATUS_INVALID_ARGUMENT;
    }

    extent = hypot(hypot(displacement_x_m, displacement_y_m),
                   displacement_z_m);
    if (!isfinite(extent)) {
        return AMCORE_STATUS_OUT_OF_RANGE;
    }

    *extent_m = extent;
    return AMCORE_STATUS_SUCCESS;
}
