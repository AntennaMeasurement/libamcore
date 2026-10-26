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
    return 0;
}
