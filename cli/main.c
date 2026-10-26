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
static int calculate_wavelength(int argc, char **argv);
static int calculate_distance(int argc, char **argv);

static const command commands[] = {
    {"help", "Show this help message", show_help},
    {"version", "Show the program version", show_version},
    {"utility", "Utility calculations", run_utility},
    {"farfield", "Far-field calculations", run_farfield},
};

static const command utility_functions[] = {
    {"wavelength", "Calculate free-space wavelength in metres",
     calculate_wavelength},
};

static const command farfield_functions[] = {
    {"distance", "Calculate minimum far-field distance in metres",
     calculate_distance},
};

static int is_help(const char *argument)
{
    return strcmp(argument, "help") == 0 || strcmp(argument, "-h") == 0 ||
           strcmp(argument, "--help") == 0;
}

static int run_module(const char *module, const command *functions,
                      size_t count, int argc, char **argv)
{
    size_t i;

    if (argc == 0 || (argc == 1 && is_help(argv[0]))) {
        printf("Usage: amcore %s <function> [arguments]\n\n", module);
        puts("Functions:");
        for (i = 0; i < count; ++i) {
            printf("  %-10s %s\n", functions[i].name,
                   functions[i].description);
        }
        printf("\nRun 'amcore %s <function> --help' for arguments.\n", module);
        return EXIT_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        if (strcmp(argv[0], functions[i].name) == 0) {
            return functions[i].handler(argc - 1, argv + 1);
        }
    }

    fprintf(stderr, "amcore %s: unknown function '%s'\n", module, argv[0]);
    fprintf(stderr, "Run 'amcore %s --help' to see available functions.\n",
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

static int parse_positive_double(const char *text, const char *name,
                                 double *value)
{
    char *end;
    double parsed;

    errno = 0;
    parsed = strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE ||
        !isfinite(parsed) || parsed <= 0.0) {
        fprintf(stderr, "amcore: %s must be a positive finite number "
                        "representable as a double; got '%s'\n", name, text);
        return EXIT_FAILURE;
    }

    *value = parsed;
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
        fputs("amcore: calculation out of range for a positive finite double\n",
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

    status = amcore_free_space_wavelength(frequency, &wavelength);
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
