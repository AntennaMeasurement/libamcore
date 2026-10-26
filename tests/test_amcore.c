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
    return 0;
}
