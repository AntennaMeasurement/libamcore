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

static const command commands[] = {
    {"help", "Show this help message", show_help},
    {"version", "Show the program version", show_version},
};

static int show_help(int argc, char **argv)
{
    size_t i;

    (void)argv;
    if (argc != 0) {
        fputs("amcore: help does not accept arguments\n", stderr);
        return EXIT_FAILURE;
    }

    puts("Usage: amcore <subcommand> [options]");
    puts("       amcore [--help | --version]");
    puts("");
    puts("Subcommands:");
    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
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

    fprintf(stderr, "amcore: unknown subcommand '%s'\n", subcommand);
    fputs("Run 'amcore help' to see available subcommands.\n", stderr);
    return EXIT_FAILURE;
}
