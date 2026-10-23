#include <amcore/amcore.h>

#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int exercise_calls(void)
{
    double output = 123.0;
    const double invalid[] = {NAN, INFINITY, -INFINITY, 0.0};
    size_t i;

    if (amcore_free_space_wavelength(1e9, &output) != AMCORE_STATUS_SUCCESS ||
        fabs(output - 0.299792458) > 1e-12) {
        return 0;
    }
    output = 123.0;
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        if (amcore_free_space_wavelength(invalid[i], &output) !=
                AMCORE_STATUS_INVALID_ARGUMENT || output != 123.0) {
            return 0;
        }
    }
    if (amcore_free_space_wavelength(1e9, NULL) != AMCORE_STATUS_INVALID_ARGUMENT ||
        amcore_free_space_wavelength(DBL_MIN, &output) != AMCORE_STATUS_OUT_OF_RANGE ||
        output != 123.0) {
        return 0;
    }
    if (amcore_far_field_distance(1e9, 1.0, &output) != AMCORE_STATUS_SUCCESS ||
        fabs(output - 2e9 / 299792458.0) > 1e-12) {
        return 0;
    }
    output = 123.0;
    return amcore_far_field_distance(1e9, -1.0, &output) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           amcore_far_field_distance(NAN, 1.0, &output) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           amcore_far_field_distance(1e9, DBL_MAX, &output) ==
               AMCORE_STATUS_OUT_OF_RANGE &&
           amcore_far_field_distance(1e9, 1.0, NULL) ==
               AMCORE_STATUS_INVALID_ARGUMENT &&
           output == 123.0;
}

int main(int argc, char **argv)
{
    double output = 123.0;

    if (argc != 2) {
        fputs("expected telemetry test mode\n", stderr);
        return 1;
    }
    if (strcmp(argv[1], "verify") == 0) {
        return exercise_calls() ? 0 : 1;
    }
    if (strcmp(argv[1], "locale") == 0) {
        if (setlocale(LC_NUMERIC, "German_Germany.1252") == NULL &&
            setlocale(LC_NUMERIC, "de_DE.UTF-8") == NULL) {
            puts("SKIP: decimal-comma locale unavailable");
            return 0;
        }
    }
    return amcore_free_space_wavelength(1e9, &output) == AMCORE_STATUS_SUCCESS &&
           fabs(output - 0.299792458) < 1e-12 ? 0 : 1;
}
