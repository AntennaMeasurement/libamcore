#include <amcore/amcore.h>

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static int test_wavelength(void)
{
    double wavelength = -1.0;
    const double expected = 0.299792458;

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
    return 0;
}
