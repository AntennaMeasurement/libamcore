/* Last content update: Unreleased (latest tag: v0.2.0) */
#include <amcore/amcore.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AMCORE_VERSION
#error "AMCORE_VERSION must be defined by the build system"
#endif

typedef int (*command_handler)(int argc, char **argv);

typedef struct command {
    const char *name;
    const char *description;
    command_handler handler;
} command;

static int show_help(int argc, char **argv);
static int show_version(int argc, char **argv);
static int run_utility(int argc, char **argv);
static int run_farfield(int argc, char **argv);
static int run_nearfield(int argc, char **argv);
static int run_nearfield_planar(int argc, char **argv);
static int run_nearfield_cylindrical(int argc, char **argv);
static int run_nearfield_spherical(int argc, char **argv);
static int calculate_wavelength(int argc, char **argv);
static int calculate_distance(int argc, char **argv);
static int calculate_line_of_sight(int argc, char **argv);
static int calculate_radio_horizon(int argc, char **argv);
static int calculate_path_loss(int argc, char **argv);
static int calculate_near_field_separation_distance(int argc, char **argv);
static int calculate_planar_ideal_step_size(int argc, char **argv);
static int calculate_planar_ideal_scan_length(int argc, char **argv);
static int calculate_planar_ideal_angle_of_view(int argc, char **argv);
static int calculate_planar_sampling_parameters(int argc, char **argv);
static int calculate_cylindrical_ideal_y_scan_length(int argc, char **argv);
static int calculate_cylindrical_ideal_y_step_size(int argc, char **argv);
static int calculate_spherical_maximum_radius_extent(int argc, char **argv);

static const command commands[] = {
    {"help", "Show this help message", show_help},
    {"version", "Show the program version", show_version},
    {"utility", "Utility calculations", run_utility},
    {"farfield", "Far-field calculations", run_farfield},
    {"nearfield", "Near-field calculations", run_nearfield},
};

static const command utility_functions[] = {
    {"wavelength", "Calculate free-space wavelength in metres",
     calculate_wavelength},
};

static const command farfield_functions[] = {
    {"distance", "Calculate minimum far-field distance in metres",
     calculate_distance},
    {"line-of-sight", "Calculate line-of-sight range in kilometres",
     calculate_line_of_sight},
    {"radio-horizon", "Calculate radio horizon in kilometres",
     calculate_radio_horizon},
    {"path-loss", "Calculate free-space path loss in decibels",
     calculate_path_loss},
};

static const command nearfield_functions[] = {
    {"separation-distance", "Calculate recommended near-field separation in metres",
     calculate_near_field_separation_distance},
    {"planar", "Planar near-field calculations", run_nearfield_planar},
    {"cylindrical", "Cylindrical near-field calculations",
     run_nearfield_cylindrical},
    {"spherical", "Spherical near-field calculations",
     run_nearfield_spherical},
};

static const command nearfield_planar_functions[] = {
    {"ideal-step-size", "Calculate ideal step size in metres",
     calculate_planar_ideal_step_size},
    {"ideal-scan-length", "Calculate ideal scan length in metres",
     calculate_planar_ideal_scan_length},
    {"ideal-angle-of-view", "Calculate ideal angle of view in degrees",
     calculate_planar_ideal_angle_of_view},
    {"sampling-parameters", "Calculate centered sampling parameters",
     calculate_planar_sampling_parameters},
};

static const command nearfield_spherical_functions[] = {
    {"maximum-radius-extent", "Calculate maximum radius extent in metres",
     calculate_spherical_maximum_radius_extent},
};

static const command nearfield_cylindrical_functions[] = {
    {"ideal-y-scan-length", "Calculate ideal cylindrical Y scan length in metres",
     calculate_cylindrical_ideal_y_scan_length},
    {"ideal-y-step-size", "Calculate ideal cylindrical Y step size in metres",
     calculate_cylindrical_ideal_y_step_size},
};

static int is_help(const char *argument)
{
    return strcmp(argument, "help") == 0 || strcmp(argument, "-h") == 0 ||
           strcmp(argument, "--help") == 0;
}

static int run_module(const char *module, const command *subcommands,
                      size_t count, int argc, char **argv)
{
    size_t i;
    size_t name_width = 0;

    if (argc == 0 || (argc == 1 && is_help(argv[0]))) {
        for (i = 0; i < count; ++i) {
            size_t current_width = strlen(subcommands[i].name);
            if (current_width > name_width) {
                name_width = current_width;
            }
        }
        printf("Usage: amcore %s <subcommand> [arguments]\n\n", module);
        puts("Subcommands:");
        for (i = 0; i < count; ++i) {
            printf("  %-*s %s\n", (int)name_width, subcommands[i].name,
                   subcommands[i].description);
        }
        puts("");
        puts("Options:");
        puts("  -h, --help  Show this help message");
        printf("\nRun 'amcore %s <subcommand> --help' for arguments.\n", module);
        return EXIT_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        if (strcmp(argv[0], subcommands[i].name) == 0) {
            return subcommands[i].handler(argc - 1, argv + 1);
        }
    }

    fprintf(stderr, "amcore %s: unknown subcommand '%s'\n", module, argv[0]);
    fprintf(stderr, "Run 'amcore %s --help' to see available subcommands.\n",
            module);
    return EXIT_FAILURE;
}

static int run_utility(int argc, char **argv)
{
    return run_module("utility", utility_functions,
                      sizeof(utility_functions) / sizeof(utility_functions[0]),
                      argc, argv);
}

static int run_farfield(int argc, char **argv)
{
    return run_module("farfield", farfield_functions,
                      sizeof(farfield_functions) / sizeof(farfield_functions[0]),
                      argc, argv);
}

static int run_nearfield(int argc, char **argv)
{
    return run_module("nearfield", nearfield_functions,
                      sizeof(nearfield_functions) / sizeof(nearfield_functions[0]),
                      argc, argv);
}

static int run_nearfield_planar(int argc, char **argv)
{
    return run_module("nearfield planar", nearfield_planar_functions,
                      sizeof(nearfield_planar_functions) /
                          sizeof(nearfield_planar_functions[0]),
                      argc, argv);
}

static int run_nearfield_spherical(int argc, char **argv)
{
    return run_module("nearfield spherical", nearfield_spherical_functions,
                      sizeof(nearfield_spherical_functions) /
                          sizeof(nearfield_spherical_functions[0]),
                      argc, argv);
}

static int run_nearfield_cylindrical(int argc, char **argv)
{
    return run_module("nearfield cylindrical",
                      nearfield_cylindrical_functions,
                      sizeof(nearfield_cylindrical_functions) /
                          sizeof(nearfield_cylindrical_functions[0]),
                      argc, argv);
}

static int parse_finite_double(const char *text, const char *name,
                               double *value)
{
    char *end;
    double parsed;

    errno = 0;
    parsed = strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE ||
        !isfinite(parsed)) {
        fprintf(stderr, "amcore: %s must be a finite number representable "
                        "as a double; got '%s'\n", name, text);
        return EXIT_FAILURE;
    }

    *value = parsed;
    return EXIT_SUCCESS;
}

static int parse_positive_double(const char *text, const char *name,
                                 double *value)
{
    if (parse_finite_double(text, name, value) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (*value <= 0.0) {
        fprintf(stderr, "amcore: %s must be a positive finite number; "
                        "got '%s'\n", name, text);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int parse_nonnegative_double(const char *text, const char *name,
                                    double *value)
{
    if (parse_finite_double(text, name, value) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (*value < 0.0) {
        fprintf(stderr, "amcore: %s must be a non-negative finite number; "
                        "got '%s'\n", name, text);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int print_result(amcore_status status, double result)
{
    switch (status) {
    case AMCORE_STATUS_SUCCESS:
        printf("%.17g\n", result);
        return EXIT_SUCCESS;
    case AMCORE_STATUS_INVALID_ARGUMENT:
        fputs("amcore: invalid calculation arguments\n", stderr);
        return EXIT_FAILURE;
    case AMCORE_STATUS_OUT_OF_RANGE:
        fputs("amcore: calculation out of range for a finite double\n",
              stderr);
        return EXIT_FAILURE;
    default:
        fprintf(stderr, "amcore: unexpected calculation status %d\n",
                (int)status);
        return EXIT_FAILURE;
    }
}

static int calculate_wavelength(int argc, char **argv)
{
    double frequency;
    double wavelength = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore utility wavelength <frequency_hz>");
        puts("");
        puts("Arguments:");
        puts("  <frequency_hz>  Frequency in hertz");
        puts("");
        puts("Options:");
        puts("  -h, --help      Show this help message and exit");
        puts("");
        puts("Calculate free-space wavelength. Frequency must be positive and finite.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore utility wavelength <frequency_hz>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_free_space_wavelength_m(frequency, &wavelength);
    return print_result(status, wavelength);
}

static int calculate_distance(int argc, char **argv)
{
    double frequency;
    double aperture;
    double distance = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore farfield distance <frequency_hz> <aperture_length_m>");
        puts("");
        puts("Arguments:");
        puts("  <frequency_hz>       Frequency in hertz");
        puts("  <aperture_length_m>  Largest aperture length in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Calculate minimum far-field distance per IEEE 149-2021, 7.1.2.");
        puts("Frequency (Hz) and largest aperture length (m) must be positive and finite.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 2) {
        fputs("Usage: amcore farfield distance <frequency_hz> "
              "<aperture_length_m>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
            EXIT_SUCCESS ||
        parse_positive_double(argv[1], "aperture_length_m", &aperture) !=
            EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_far_field_distance(frequency, aperture, &distance);
    return print_result(status, distance);
}

static int calculate_line_of_sight(int argc, char **argv)
{
    double height;
    double range = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore farfield line-of-sight <height_m>");
        puts("");
        puts("Arguments:");
        puts("  <height_m>  Height in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Calculate approximate line-of-sight range.");
        puts("Height must be non-negative and finite.");
        puts("Output: one numeric value in kilometres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore farfield line-of-sight <height_m>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_nonnegative_double(argv[0], "height_m", &height) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_far_field_line_of_sight_km(height, &range);
    return print_result(status, range);
}

static int calculate_radio_horizon(int argc, char **argv)
{
    double height;
    double range = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore farfield radio-horizon <height_m>");
        puts("");
        puts("Arguments:");
        puts("  <height_m>  Height in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Calculate approximate radio horizon.");
        puts("Height must be non-negative and finite.");
        puts("Output: one numeric value in kilometres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore farfield radio-horizon <height_m>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_nonnegative_double(argv[0], "height_m", &height) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_far_field_radio_horizon_km(height, &range);
    return print_result(status, range);
}

static int calculate_path_loss(int argc, char **argv)
{
    double frequency;
    double range;
    double transmit_gain;
    double receive_gain;
    double path_loss = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore farfield path-loss <frequency_mhz> <range_km> "
             "<transmit_gain_db> <receive_gain_db>");
        puts("");
        puts("Arguments:");
        puts("  <frequency_mhz>     Frequency in megahertz");
        puts("  <range_km>          Range in kilometres");
        puts("  <transmit_gain_db>  Transmit gain in decibels");
        puts("  <receive_gain_db>   Receive gain in decibels");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Calculate free-space path loss under far-field conditions.");
        puts("Frequency and range must be positive and finite; gains must be finite.");
        puts("Output: one numeric value in decibels.");
        return EXIT_SUCCESS;
    }
    if (argc != 4) {
        fputs("Usage: amcore farfield path-loss <frequency_mhz> <range_km> "
              "<transmit_gain_db> <receive_gain_db>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_mhz", &frequency) !=
            EXIT_SUCCESS ||
        parse_positive_double(argv[1], "range_km", &range) != EXIT_SUCCESS ||
        parse_finite_double(argv[2], "transmit_gain_db", &transmit_gain) !=
            EXIT_SUCCESS ||
        parse_finite_double(argv[3], "receive_gain_db", &receive_gain) !=
            EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_far_field_path_loss_db(frequency, range, transmit_gain,
                                           receive_gain, &path_loss);
    return print_result(status, path_loss);
}

static int calculate_planar_ideal_step_size(int argc, char **argv)
{
    double frequency;
    double spacing = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield planar ideal-step-size <frequency_hz>");
        puts("Calculate half-wavelength ideal step size.");
        puts("");
        puts("Arguments:");
        puts("  <frequency_hz> Frequency in hertz");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Frequency must be positive and finite.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore nearfield planar ideal-step-size "
              "<frequency_hz>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status =
        amcore_near_field_planar_ideal_step_size_m(frequency, &spacing);
    return print_result(status, spacing);
}

static int calculate_cylindrical_ideal_y_step_size(int argc, char **argv)
{
    double frequency;
    double step_size = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield cylindrical ideal-y-step-size <frequency_hz>");
        puts("Calculate half-wavelength Y spacing without rounding.");
        puts("");
        puts("Arguments:");
        puts("  <frequency_hz> Frequency in hertz");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Frequency must be positive and finite.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore nearfield cylindrical ideal-y-step-size "
              "<frequency_hz>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_cylindrical_ideal_y_step_size_m(
        frequency, &step_size);
    return print_result(status, step_size);
}

static int calculate_near_field_separation_distance(int argc, char **argv)
{
    double frequency;
    double separation = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield separation-distance <frequency_hz>");
        puts("Calculate the recommended five-wavelength measurement separation.");
        puts("Arguments:");
        puts("  <frequency_hz> Frequency in hertz");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Frequency must be positive and finite.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 1) {
        fputs("Usage: amcore nearfield separation-distance <frequency_hz>\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status =
        amcore_near_field_separation_distance_m(frequency, &separation);
    return print_result(status, separation);
}

static int calculate_planar_ideal_scan_length(int argc, char **argv)
{
    double aperture;
    double separation;
    double angle;
    double scan_length = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield planar ideal-scan-length "
             "<aperture_cross_section_m> <probe_separation_m> "
             "<angle_of_view_deg>");
        puts("Calculate the required scan length for a desired angle of view.");
        puts("Arguments:");
        puts("  <aperture_cross_section_m> Aperture cross-section in metres");
        puts("  <probe_separation_m>       Probe separation in metres");
        puts("  <angle_of_view_deg>        Angle of view in degrees");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Aperture length must be non-negative, separation positive, and "
             "angle in [0, 90) degrees.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 3) {
        fputs("Usage: amcore nearfield planar ideal-scan-length "
              "<aperture_cross_section_m> <probe_separation_m> "
              "<angle_of_view_deg>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_nonnegative_double(argv[0], "aperture_cross_section_m",
                                 &aperture) != EXIT_SUCCESS ||
        parse_positive_double(argv[1], "probe_separation_m", &separation) !=
            EXIT_SUCCESS ||
        parse_nonnegative_double(argv[2], "angle_of_view_deg", &angle) !=
            EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_planar_ideal_scan_length_m(
        aperture, separation, angle, &scan_length);
    return print_result(status, scan_length);
}

static int calculate_planar_ideal_angle_of_view(int argc, char **argv)
{
    double aperture;
    double separation;
    double scan_length;
    double angle = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield planar ideal-angle-of-view "
             "<aperture_cross_section_m> <probe_separation_m> "
             "<scan_length_m>");
        puts("Calculate the ideal angle of view for a centered scan.");
        puts("Arguments:");
        puts("  <aperture_cross_section_m> Aperture cross-section in metres");
        puts("  <probe_separation_m>      Probe separation in metres");
        puts("  <scan_length_m>           Scan length in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Aperture and scan lengths must be non-negative; separation "
             "must be positive.");
        puts("Output: one numeric value in degrees.");
        return EXIT_SUCCESS;
    }
    if (argc != 3) {
        fputs("Usage: amcore nearfield planar ideal-angle-of-view "
              "<aperture_cross_section_m> <probe_separation_m> "
              "<scan_length_m>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_nonnegative_double(argv[0], "aperture_cross_section_m",
                                 &aperture) != EXIT_SUCCESS ||
        parse_positive_double(argv[1], "probe_separation_m", &separation) !=
            EXIT_SUCCESS ||
        parse_nonnegative_double(argv[2], "scan_length_m", &scan_length) !=
            EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_planar_ideal_angle_of_view_deg(
        aperture, separation, scan_length, &angle);
    return print_result(status, angle);
}

static int calculate_planar_sampling_parameters(int argc, char **argv)
{
    double frequency;
    double minimum_scan_length;
    double scan_start = 0.0;
    double scan_stop = 0.0;
    size_t sample_count = 0;
    size_t spacing_mm = 0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield planar sampling-parameters "
             "<frequency_hz> <minimum_scan_length_m>");
        puts("Calculate centered scan endpoints, sample count, and spacing.");
        puts("Arguments:");
        puts("  <frequency_hz>          Frequency in hertz");
        puts("  <minimum_scan_length_m> Minimum scan length in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Frequency must be positive and scan length non-negative; both "
             "must be finite.");
        puts("Output: scan start (m), scan stop (m), sample count, and "
             "sampling spacing (mm).");
        return EXIT_SUCCESS;
    }
    if (argc != 2) {
        fputs("Usage: amcore nearfield planar sampling-parameters "
              "<frequency_hz> <minimum_scan_length_m>\n", stderr);
        return EXIT_FAILURE;
    }
    if (parse_positive_double(argv[0], "frequency_hz", &frequency) !=
            EXIT_SUCCESS ||
        parse_nonnegative_double(argv[1], "minimum_scan_length_m",
                                 &minimum_scan_length) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_planar_sampling_parameters(
        frequency, minimum_scan_length, &scan_start, &scan_stop,
        &sample_count, &spacing_mm);
    if (status != AMCORE_STATUS_SUCCESS) {
        return print_result(status, 0.0);
    }
    printf("%.17g %.17g %zu %zu\n", scan_start, scan_stop, sample_count,
           spacing_mm);
    return EXIT_SUCCESS;
}

static int calculate_spherical_maximum_radius_extent(int argc, char **argv)
{
    double displacement_x;
    double displacement_y;
    double displacement_z;
    double extent = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield spherical maximum-radius-extent "
             "<displacement_x_m> <displacement_y_m> <displacement_z_m>");
        puts("Calculate the Euclidean maximum radius extent.");
        puts("");
        puts("Arguments:");
        puts("  <displacement_x_m> Displacement in the x direction in metres");
        puts("  <displacement_y_m> Displacement in the y direction in metres");
        puts("  <displacement_z_m> Displacement in the z direction in metres");
        puts("");
        puts("Options:");
        puts("  -h, --help         Show this help message and exit");
        puts("");
        puts("Displacements must be finite; signed values are accepted.");
        puts("Output: one numeric value in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 3) {
        fputs("Usage: amcore nearfield spherical maximum-radius-extent "
              "<displacement_x_m> <displacement_y_m> <displacement_z_m>\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (parse_finite_double(argv[0], "displacement_x_m", &displacement_x) !=
            EXIT_SUCCESS ||
        parse_finite_double(argv[1], "displacement_y_m", &displacement_y) !=
            EXIT_SUCCESS ||
        parse_finite_double(argv[2], "displacement_z_m", &displacement_z) !=
            EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_spherical_maximum_radius_extent_m(
        displacement_x, displacement_y, displacement_z, &extent);
    return print_result(status, extent);
}

static int calculate_cylindrical_ideal_y_scan_length(int argc, char **argv)
{
    double antenna_height;
    double probe_longest_edge;
    double probe_radius;
    double maximum_radial_extent;
    double azimuth_max;
    double elevation_max;
    double scan_length = 0.0;
    amcore_status status;

    if (argc == 1 && is_help(argv[0])) {
        puts("Usage: amcore nearfield cylindrical ideal-y-scan-length "
             "<antenna_height_m> <probe_longest_edge_m> <probe_radius_m> "
             "<maximum_radial_extent_m> <azimuth_max_deg> "
             "<elevation_max_deg>");
        puts("");
        puts("Arguments:");
        puts("  <antenna_height_m>        Antenna height in metres");
        puts("  <probe_longest_edge_m>    Longest edge of the probe in metres");
        puts("  <probe_radius_m>          Probe radius in metres");
        puts("  <maximum_radial_extent_m> Maximum radial extent in metres");
        puts("  <azimuth_max_deg>         Maximum azimuth angle in degrees");
        puts("  <elevation_max_deg>       Maximum elevation angle in degrees");
        puts("");
        puts("Options:");
        puts("  -h, --help                Show this help message and exit");
        puts("");
        puts("Calculate the ideal cylindrical Y scan length using the "
             "geometry depicted in the NSI2000 manual.");
        puts("Lengths must be non-negative, probe radius greater than "
             "maximum radial extent, azimuth in [0, 180], and elevation in "
             "[0, 90); all inputs must be finite.");
        puts("Output: scan length in metres.");
        return EXIT_SUCCESS;
    }
    if (argc != 6) {
        fputs("Usage: amcore nearfield cylindrical ideal-y-scan-length "
              "<antenna_height_m> <probe_longest_edge_m> <probe_radius_m> "
              "<maximum_radial_extent_m> <azimuth_max_deg> "
              "<elevation_max_deg>\n", stderr);

        return EXIT_FAILURE;
    }
    if (parse_nonnegative_double(argv[0], "antenna_height_m",
                                 &antenna_height) != EXIT_SUCCESS ||
        parse_nonnegative_double(argv[1], "probe_longest_edge_m",
                                 &probe_longest_edge) != EXIT_SUCCESS ||
        parse_nonnegative_double(argv[2], "probe_radius_m",
                                 &probe_radius) != EXIT_SUCCESS ||
        parse_nonnegative_double(argv[3], "maximum_radial_extent_m",
                                 &maximum_radial_extent) != EXIT_SUCCESS ||
        parse_nonnegative_double(argv[4], "azimuth_max_deg",
                                 &azimuth_max) != EXIT_SUCCESS ||
        parse_nonnegative_double(argv[5], "elevation_max_deg",
                                 &elevation_max) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    status = amcore_near_field_cylindrical_ideal_y_scan_length_m(
        antenna_height, probe_longest_edge, probe_radius,
        maximum_radial_extent, azimuth_max, elevation_max, &scan_length);
    return print_result(status, scan_length);
}

static int show_help(int argc, char **argv)
{
    size_t i;

    (void)argv;
    if (argc != 0) {
        fputs("amcore: help does not accept arguments\n", stderr);
        return EXIT_FAILURE;
    }

    puts("Usage: amcore <module> [options]");
    puts("       amcore [--help | --version]");
    puts("");
    puts("Modules:");
    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        if (commands[i].handler == show_help ||
            commands[i].handler == show_version) {
            continue;
        }
        printf("  %-10s %s\n", commands[i].name, commands[i].description);
    }
    puts("");
    puts("Options:");
    puts("  -h, --help     Show this help message");
    puts("  -V, --version  Show the program version");
    return EXIT_SUCCESS;
}

static int show_version(int argc, char **argv)
{
    (void)argv;
    if (argc != 0) {
        fputs("amcore: version does not accept arguments\n", stderr);
        return EXIT_FAILURE;
    }

    printf("amcore %s\n", AMCORE_VERSION);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    size_t i;
    const char *subcommand;

    if (argc == 1) {
        return show_help(0, NULL);
    }

    subcommand = argv[1];
    if (strcmp(subcommand, "-h") == 0 || strcmp(subcommand, "--help") == 0) {
        return show_help(argc - 2, argv + 2);
    }
    if (strcmp(subcommand, "-V") == 0 ||
        strcmp(subcommand, "--version") == 0) {
        return show_version(argc - 2, argv + 2);
    }

    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        if (strcmp(subcommand, commands[i].name) == 0) {
            return commands[i].handler(argc - 2, argv + 2);
        }
    }

    fprintf(stderr, "amcore: unknown module '%s'\n", subcommand);
    fputs("Run 'amcore help' to see available modules.\n", stderr);
    return EXIT_FAILURE;
}
