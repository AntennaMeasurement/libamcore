/* Last content update tag: v0.1.0 */
#include <amcore/amcore.h>

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static int test_wavelength(void)
{
    double wavelength = -1.0;
    const double expected = 0.299792458;

    if (speed_of_light_m_per_s != 299792458.0) {
        return 1;
    }
    if (amcore_free_space_wavelength(1.0e9, &wavelength) !=
        AMCORE_STATUS_SUCCESS) {
        return 1;
    }
    if (fabs(wavelength - expected) > 1.0e-12) {
        return 1;
    }
    return 0;
}

static int test_invalid_arguments(void)
{
    const double invalid_frequencies[] = {0.0, -1.0, INFINITY, NAN};
    double wavelength = 123.0;
    size_t i;

    if (amcore_free_space_wavelength(1.0e9, NULL) !=
        AMCORE_STATUS_INVALID_ARGUMENT) {
        return 1;
    }

    for (i = 0; i < sizeof(invalid_frequencies) / sizeof(invalid_frequencies[0]);
         ++i) {
        if (amcore_free_space_wavelength(invalid_frequencies[i], &wavelength) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            wavelength != 123.0) {
            return 1;
        }
    }
    return 0;
}

static int test_out_of_range(void)
{
    double wavelength = 123.0;

    if (amcore_free_space_wavelength(DBL_MIN, &wavelength) !=
            AMCORE_STATUS_OUT_OF_RANGE ||
        wavelength != 123.0) {
        return 1;
    }
    return 0;
}

static int test_far_field_distance(void)
{
    const struct {
        double frequency_hz;
        double aperture_length_m;
        double expected_m;
    } cases[] = {
        {299792458.0, 0.1, 1.6},
        {299792458.0, 0.5, 2.5},
        {299792458.0, 4.0, 32.0},
        {299792458.0, 0.32, 1.6},
        {299792458.0, 2.5, 12.5},
        {1.0e9, 1.0, 2.0e9 / 299792458.0},
        {9.0e9, 0.3, 5.4037383422100627},
        {299792458.0 / 1.0e200, 4.0e200, 3.2e201},
        {299792458.0 / 1.0e-200, 4.0e-200, 3.2e-199},
        {DBL_MAX, DBL_MIN, 1.6 * (299792458.0 / DBL_MAX)}
    };
    double distance = -1.0;
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (amcore_far_field_distance(cases[i].frequency_hz,
                                     cases[i].aperture_length_m, &distance) !=
                AMCORE_STATUS_SUCCESS ||
            !isfinite(distance) ||
            fabs(distance / cases[i].expected_m - 1.0) > 1.0e-12) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_invalid_arguments(void)
{
    const double invalid_values[] = {0.0, -1.0, INFINITY, -INFINITY, NAN};
    double distance = 123.0;
    size_t i;

    if (amcore_far_field_distance(1.0e9, 1.0, NULL) !=
        AMCORE_STATUS_INVALID_ARGUMENT) {
        return 1;
    }
    for (i = 0; i < sizeof(invalid_values) / sizeof(invalid_values[0]); ++i) {
        if (amcore_far_field_distance(invalid_values[i], 1.0, &distance) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            distance != 123.0) {
            return 1;
        }
        if (amcore_far_field_distance(1.0e9, invalid_values[i], &distance) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            distance != 123.0) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_out_of_range(void)
{
    const struct {
        double frequency_hz;
        double aperture_length_m;
    } cases[] = {
        {DBL_MIN, 1.0},
        {299792458.0 / (DBL_MAX * 0.75), 1.0},
        {299792458.0, DBL_MAX},
        {299792458.0, 1.0e200},
        {DBL_MAX, 1.0e10}
    };
    double distance = 123.0;
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (amcore_far_field_distance(cases[i].frequency_hz,
                                     cases[i].aperture_length_m, &distance) !=
                AMCORE_STATUS_OUT_OF_RANGE ||
            distance != 123.0) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_horizons(void)
{
    const struct {
        double height_m;
        double line_of_sight_km;
        double radio_horizon_km;
    } cases[] = {
        {0.0, 0.0, 0.0},
        {1.0, 3.57, 4.12},
        {10.0, 3.57 * sqrt(10.0), 4.12 * sqrt(10.0)},
        {100.0, 35.7, 41.2},
        {DBL_MAX, 3.57 * sqrt(DBL_MAX), 4.12 * sqrt(DBL_MAX)}
    };
    double range = -1.0;
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (amcore_far_field_line_of_sight_km(cases[i].height_m, &range) !=
                AMCORE_STATUS_SUCCESS ||
            fabs(range - cases[i].line_of_sight_km) >
                1.0e-12 * cases[i].line_of_sight_km + 1.0e-12) {
            return 1;
        }
        if (amcore_far_field_radio_horizon_km(cases[i].height_m, &range) !=
                AMCORE_STATUS_SUCCESS ||
            fabs(range - cases[i].radio_horizon_km) >
                1.0e-12 * cases[i].radio_horizon_km + 1.0e-12) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_horizon_invalid_arguments(void)
{
    const double invalid_heights[] = {-1.0, INFINITY, -INFINITY, NAN};
    double range = 123.0;
    size_t i;

    if (amcore_far_field_line_of_sight_km(1.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_far_field_radio_horizon_km(1.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT) {
        return 1;
    }
    for (i = 0; i < sizeof(invalid_heights) / sizeof(invalid_heights[0]); ++i) {
        if (amcore_far_field_line_of_sight_km(invalid_heights[i], &range) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            range != 123.0 ||
            amcore_far_field_radio_horizon_km(invalid_heights[i], &range) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            range != 123.0) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_path_loss(void)
{
    const struct {
        double frequency_mhz;
        double range_km;
        double transmit_gain_db;
        double receive_gain_db;
        double expected_db;
    } cases[] = {
        {100.0, 1.0, 0.0, 0.0, 72.45},
        {100.0, 1.0, 10.0, 10.0, 52.45},
        {500.0, 11.0, 20.0, 0.0, 87.257253789884885},
        {100.0, 1.0, DBL_MAX, -DBL_MAX, 72.45},
        {1000.0, 2.0, 2.5, -3.0, 32.45 + 20.0 * log10(2000.0) - 2.5 + 3.0},
        {DBL_MAX, DBL_MAX, 0.0, 0.0,
         32.45 + 40.0 * log10(DBL_MAX)}
    };
    double path_loss = 0.0;
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (amcore_far_field_path_loss_db(
                cases[i].frequency_mhz, cases[i].range_km,
                cases[i].transmit_gain_db, cases[i].receive_gain_db,
                &path_loss) != AMCORE_STATUS_SUCCESS ||
            !isfinite(path_loss) ||
            fabs(path_loss - cases[i].expected_db) > 1.0e-12) {
            return 1;
        }
    }
    if (amcore_far_field_path_loss_db(100.0, 1.0, 100.0, 100.0,
                                     &path_loss) != AMCORE_STATUS_SUCCESS ||
        fabs(path_loss - (-127.55)) > 1.0e-12) {
        return 1;
    }
    return 0;
}

static int test_far_field_path_loss_invalid_arguments(void)
{
    const double invalid_values[] = {0.0, -1.0, INFINITY, -INFINITY, NAN};
    const double invalid_gains[] = {INFINITY, -INFINITY, NAN};
    double path_loss = 123.0;
    size_t i;

    if (amcore_far_field_path_loss_db(100.0, 1.0, 0.0, 0.0, NULL) !=
        AMCORE_STATUS_INVALID_ARGUMENT) {
        return 1;
    }
    for (i = 0; i < sizeof(invalid_values) / sizeof(invalid_values[0]); ++i) {
        if (amcore_far_field_path_loss_db(invalid_values[i], 1.0, 0.0, 0.0,
                                         &path_loss) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            path_loss != 123.0) {
            return 1;
        }
        if (amcore_far_field_path_loss_db(100.0, invalid_values[i], 0.0, 0.0,
                                         &path_loss) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            path_loss != 123.0) {
            return 1;
        }
    }
    for (i = 0; i < sizeof(invalid_gains) / sizeof(invalid_gains[0]); ++i) {
        if (amcore_far_field_path_loss_db(100.0, 1.0, invalid_gains[i], 0.0,
                                         &path_loss) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            path_loss != 123.0 ||
            amcore_far_field_path_loss_db(100.0, 1.0, 0.0, invalid_gains[i],
                                         &path_loss) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            path_loss != 123.0) {
            return 1;
        }
    }
    return 0;
}

static int test_far_field_path_loss_out_of_range(void)
{
    double path_loss = 123.0;

    if (amcore_far_field_path_loss_db(1.0, 1.0, -DBL_MAX, -DBL_MAX,
                                     &path_loss) !=
            AMCORE_STATUS_OUT_OF_RANGE ||
        path_loss != 123.0) {
        return 1;
    }
    return 0;
}

static int test_near_field_planar(void)
{
    double spacing = -1.0;
    double scan_length = -1.0;
    double angle = -1.0;

    if (amcore_near_field_planar_sampling_spacing_m(299792458.0,
                                                     &spacing) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(spacing - 0.5) > 1.0e-12) {
        return 1;
    }
    if (amcore_near_field_planar_scan_length_m(1.0, 2.0, 45.0,
                                               &scan_length) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(scan_length - 5.0) > 1.0e-12) {
        return 1;
    }
    if (amcore_near_field_planar_angle_of_view_deg(1.0, 2.0, 5.0,
                                                  &angle) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(angle - 45.0) > 1.0e-12) {
        return 1;
    }
    if (amcore_near_field_planar_angle_of_view_deg(1.0, 2.0, 0.0,
                                                  &angle) !=
            AMCORE_STATUS_SUCCESS ||
        angle >= 0.0) {
        return 1;
    }
    if (amcore_near_field_planar_angle_of_view_deg(
            0.0, nextafter(0.0, 1.0), nextafter(0.0, 1.0), &angle) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(angle - atan2(1.0, 2.0) * (180.0 / acos(-1.0))) > 1.0e-12) {
        return 1;
    }
    return 0;
}

static int test_near_field_attached_measurement_cases(void)
{
    double separation = -1.0;
    double spacing = -1.0;
    double scan_length = -1.0;
    double angle = -1.0;
    double extent = -1.0;
    double scan_start = 0.0;
    double scan_stop = 0.0;
    size_t sample_count = 0;
    size_t spacing_mm = 0;

    if (amcore_near_field_separation_distance_m(10.0e9, &separation) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(separation - 0.150) > 0.0005) {
        return 1;
    }
    if (amcore_near_field_planar_sampling_spacing_m(3.0e9, &spacing) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(spacing - 0.050) > 0.0005) {
        return 1;
    }
    if (amcore_near_field_planar_scan_length_m(20.0, 10.0, 60.0,
                                               &scan_length) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(scan_length - 54.64) > 0.005) {
        return 1;
    }
    if (amcore_near_field_planar_sampling_parameters(
            3.0e9, 1.0, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_SUCCESS ||
        scan_start != -0.539 || scan_stop != 0.539 || sample_count != 23 ||
        spacing_mm != 49) {
        return 1;
    }
    if (amcore_near_field_planar_angle_of_view_deg(20.0, 10.0, 54.64,
                                                  &angle) !=
            AMCORE_STATUS_SUCCESS ||
        fabs(angle - 60.0) > 0.005) {
        return 1;
    }
    if (amcore_near_field_spherical_maximum_radius_extent_m(
            3.0, 4.0, 12.0, &extent) != AMCORE_STATUS_SUCCESS ||
        fabs(extent - 13.0) > 0.005) {
        return 1;
    }

    return 0;
}

static int test_near_field_sampling_parameters(void)
{
    double scan_start = 123.0;
    double scan_stop = 456.0;
    size_t sample_count = 789;
    size_t spacing_mm = 321;

    if (amcore_near_field_planar_sampling_parameters(
            299792458.0, 1.0, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_SUCCESS ||
        scan_start != -0.5 || scan_stop != 0.5 || sample_count != 3 ||
        spacing_mm != 500) {
        return 1;
    }
    if (amcore_near_field_planar_sampling_parameters(
            299792458.0 / 0.6, 0.8, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_SUCCESS ||
        scan_start != -0.6 || scan_stop != 0.6 || sample_count != 5 ||
        spacing_mm != 300) {
        return 1;
    }
    if (amcore_near_field_planar_sampling_parameters(
            299792458.0, 0.0, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_SUCCESS ||
        scan_start != 0.0 || scan_stop != 0.0 || sample_count != 1 ||
        spacing_mm != 500) {
        return 1;
    }
    return 0;
}

static int test_near_field_invalid_arguments(void)
{
    const double invalid_values[] = {-1.0, INFINITY, -INFINITY, NAN};
    double result = 123.0;
    double scan_start = 123.0;
    double scan_stop = 456.0;
    size_t sample_count = 789;
    size_t spacing_mm = 321;
    size_t i;

    if (amcore_near_field_planar_sampling_spacing_m(1.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_separation_distance_m(1.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_scan_length_m(1.0, 1.0, 45.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_angle_of_view_deg(1.0, 1.0, 1.0, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_spherical_maximum_radius_extent_m(
            1.0, 1.0, 1.0, NULL) != AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_sampling_parameters(
            1.0, 1.0, NULL, &scan_stop, &sample_count, &spacing_mm) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_sampling_parameters(
            1.0, 1.0, &scan_start, NULL, &sample_count, &spacing_mm) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_sampling_parameters(
            1.0, 1.0, &scan_start, &scan_stop, NULL, &spacing_mm) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_near_field_planar_sampling_parameters(
            1.0, 1.0, &scan_start, &scan_stop, &sample_count, NULL) !=
            AMCORE_STATUS_INVALID_ARGUMENT) {
        return 1;
    }

    if (amcore_near_field_planar_sampling_spacing_m(0.0, &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0 ||
        amcore_near_field_separation_distance_m(0.0, &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0 ||
        amcore_near_field_planar_scan_length_m(1.0, 0.0, 45.0, &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0 ||
        amcore_near_field_planar_angle_of_view_deg(1.0, 0.0, 1.0,
                                                  &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0 ||
        amcore_near_field_planar_sampling_parameters(
            299792458.0, -1.0, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_INVALID_ARGUMENT ||
        scan_start != 123.0 || scan_stop != 456.0 || sample_count != 789 ||
        spacing_mm != 321) {
        return 1;
    }

    for (i = 0; i < sizeof(invalid_values) / sizeof(invalid_values[0]); ++i) {
        if (amcore_near_field_planar_sampling_spacing_m(invalid_values[i],
                                                        &result) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            result != 123.0) {
            return 1;
        }
        if (amcore_near_field_separation_distance_m(invalid_values[i],
                                                    &result) !=
                AMCORE_STATUS_INVALID_ARGUMENT ||
            result != 123.0) {
            return 1;
        }
        if (i > 0 &&
            (amcore_near_field_spherical_maximum_radius_extent_m(
                 invalid_values[i], 0.0, 0.0, &result) !=
                 AMCORE_STATUS_INVALID_ARGUMENT ||
             result != 123.0)) {
            return 1;
        }
    }
    if (amcore_near_field_planar_scan_length_m(1.0, 1.0, 90.0, &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0 ||
        amcore_near_field_planar_scan_length_m(1.0, 1.0, -1.0, &result) !=
            AMCORE_STATUS_INVALID_ARGUMENT ||
        result != 123.0) {
        return 1;
    }
    return 0;
}

static int test_near_field_out_of_range(void)
{
    double result = 123.0;
    double scan_start = 1.0;
    double scan_stop = 2.0;
    size_t sample_count = 3;
    size_t spacing_mm = 4;

    if (amcore_near_field_planar_scan_length_m(DBL_MAX, DBL_MAX, 45.0,
                                               &result) !=
            AMCORE_STATUS_OUT_OF_RANGE ||
        result != 123.0 ||
        amcore_near_field_separation_distance_m(DBL_MIN, &result) !=
            AMCORE_STATUS_OUT_OF_RANGE ||
        result != 123.0 ||
        amcore_near_field_spherical_maximum_radius_extent_m(
            DBL_MAX, DBL_MAX, DBL_MAX, &result) != AMCORE_STATUS_OUT_OF_RANGE ||
        result != 123.0 ||
        amcore_near_field_planar_sampling_parameters(
            2.0e11, 1.0, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_OUT_OF_RANGE ||
        scan_start != 1.0 || scan_stop != 2.0 || sample_count != 3 ||
        spacing_mm != 4 ||
        amcore_near_field_planar_sampling_parameters(
            299792458.0, DBL_MAX, &scan_start, &scan_stop, &sample_count,
            &spacing_mm) != AMCORE_STATUS_OUT_OF_RANGE ||
        scan_start != 1.0 || scan_stop != 2.0 || sample_count != 3 ||
        spacing_mm != 4) {
        return 1;
    }
    return 0;
}

static int test_near_field_spherical(void)
{
    double extent = -1.0;

    if (amcore_near_field_spherical_maximum_radius_extent_m(
            3.0, -4.0, 12.0, &extent) != AMCORE_STATUS_SUCCESS ||
        fabs(extent - 13.0) > 1.0e-12) {
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_wavelength() != 0) {
        fputs("test_wavelength failed\n", stderr);
        return 1;
    }
    if (test_invalid_arguments() != 0) {
        fputs("test_invalid_arguments failed\n", stderr);
        return 1;
    }
    if (test_out_of_range() != 0) {
        fputs("test_out_of_range failed\n", stderr);
        return 1;
    }
    if (test_far_field_distance() != 0) {
        fputs("test_far_field_distance failed\n", stderr);
        return 1;
    }
    if (test_far_field_invalid_arguments() != 0) {
        fputs("test_far_field_invalid_arguments failed\n", stderr);
        return 1;
    }
    if (test_far_field_out_of_range() != 0) {
        fputs("test_far_field_out_of_range failed\n", stderr);
        return 1;
    }
    if (test_far_field_horizons() != 0) {
        fputs("test_far_field_horizons failed\n", stderr);
        return 1;
    }
    if (test_far_field_horizon_invalid_arguments() != 0) {
        fputs("test_far_field_horizon_invalid_arguments failed\n", stderr);
        return 1;
    }
    if (test_far_field_path_loss() != 0) {
        fputs("test_far_field_path_loss failed\n", stderr);
        return 1;
    }
    if (test_far_field_path_loss_invalid_arguments() != 0) {
        fputs("test_far_field_path_loss_invalid_arguments failed\n", stderr);
        return 1;
    }
    if (test_far_field_path_loss_out_of_range() != 0) {
        fputs("test_far_field_path_loss_out_of_range failed\n", stderr);
        return 1;
    }
    if (test_near_field_planar() != 0) {
        fputs("test_near_field_planar failed\n", stderr);
        return 1;
    }
    if (test_near_field_attached_measurement_cases() != 0) {
        fputs("test_near_field_attached_measurement_cases failed\n", stderr);
        return 1;
    }
    if (test_near_field_sampling_parameters() != 0) {
        fputs("test_near_field_sampling_parameters failed\n", stderr);
        return 1;
    }
    if (test_near_field_invalid_arguments() != 0) {
        fputs("test_near_field_invalid_arguments failed\n", stderr);
        return 1;
    }
    if (test_near_field_out_of_range() != 0) {
        fputs("test_near_field_out_of_range failed\n", stderr);
        return 1;
    }
    if (test_near_field_spherical() != 0) {
        fputs("test_near_field_spherical failed\n", stderr);
        return 1;
    }
    return 0;
}
