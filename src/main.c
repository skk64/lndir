#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lndir.h"


void print_help(const char *prog_name) {
    const char *help_string =
        "Usage: %s [OPTIONS] <source_directory> <target_directory>\n"
        "Recursively create hard links for all files in <source_directory> "
        "within <target_directory>.\n"
        "Will create duplicate directories in <target_directory>.\n"
        "\n"
        "Options:\n"
        "  -h, --help        Show this help message and exit\n"
        "\n"
        "Example:\n"
        "  %s /path/to/source /path/to/target\n"
        "  %s relative/source relative/target\n";
    printf(help_string, prog_name, prog_name, prog_name);
}


int main(int argc, char *argv[]) {

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <target_directory>\n",
                argv[0]);
        fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* input = argv[1];
    char* output = argv[2];

    hardlink_directory_structure(input, output);

    // StringListIter iter = StringListIter_new(&file_list);
    // char* filename;
    // printf("\n\n");
    // while ( (filename = StringListIter_next(&iter)) != NULL) {
    //     printf("file: %s\n", filename);
    // }
}

