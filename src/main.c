#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lndir.h"

#ifndef VERSION
#define VERSION "unknown"
#endif

#ifndef debug_printf
#ifdef DEBUG
#define debug_printf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#else
#define debug_printf(fmt, ...)
#endif
#endif

void print_version() {
    printf("%s\n", VERSION);
}

void print_help(const char* prog_name) {
    const char* help_string =
        "Usage: %s [OPTIONS] <source_directory> <target_directory>\n"
        "Will duplicate the directory structure of <source_directory> in <target_directory>,\n"
        "and create hard links for all files in <source_directory>"
        "within <target_directory>.\n"
        "\n"
        "Options:\n"
        "  -h, --help        Print this help message\n"
        "  -v, --version     Print the version\n"
        "\n"
        "Example:\n"
        "  %s /path/to/source /path/to/target\n"
        "  %s relative/source relative/target\n";
    printf(help_string, prog_name, prog_name, prog_name);
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
        print_version();
        exit(EXIT_SUCCESS);
    }
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <target_directory>\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* input = argv[1];
    char* output = argv[2];

    enum lndir_result result = hardlink_directory_structure(input, output);
    switch (result) {
    case (LNDIR_SUCCESS):
        debug_printf("Success\n");
        break;
    case (LNDIR_SRC_OPEN):
        printf("Source directory (%s) couldn't be opened:  %s \n", input, strerror(errno));
        break;
    case (LNDIR_SRC_STAT):
        printf("Source directory couldn't be read:  %s \n", strerror(errno));
        break;
    case (LNDIR_DEST_CREATE):
        printf("Destination directory couldn't be created: %s \n", strerror(errno));
        break;
    case (LNDIR_DEST_OPEN):
        printf("Destination directory couldn't be opened:  %s \n", strerror(errno));
        break;
    case (LNDIR_IO_URING):
        printf("io_uring couldn't be initialised:  %s \n", strerror(errno));
        break;
    default:
        debug_printf("Unexpected result (%d), errno %d: %s\n", result, errno, strerror(errno));
        break;
    }
    
}
