#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

/// Creates the directory dest_dir, and links all files from src_dir to
/// dest_dir recursively All links are hard links, as a symbolic link of the
/// directory can be done with just ln -s
///
/// Will return failure if the src_dir cannot be opened
/// Return 0 on success, or -1 on failure, with errno set
int link_all_files_in_dir(const char *src_dir, const char *dest_dir) {
    // Ensure the source directory exists

    // don't create endless recursive directory tree
    const size_t src_len = strlen(src_dir);
    if (strncmp(dest_dir, src_dir, src_len) == 0) {
        fprintf(stderr, "\tCannot create link inside source directory, skipping: %s\n", src_dir);
        return -1;
    }

    struct stat src_dir_st;
    if (stat(src_dir, &src_dir_st) == -1 || !S_ISDIR(src_dir_st.st_mode)) {
        fprintf(stderr, "%d: %s\n\tError opening source directory: %s\n", errno,
                strerror(errno), src_dir);
        return -1;
    }

    // Create target directory with source directory permissions
    if (mkdir(dest_dir, src_dir_st.st_mode) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "%d: %s\n\tError creating target directory: %s\n",
                    errno, strerror(errno), src_dir);
            return -1;
        }
    }

    // Directory iterator
    DIR *dir = opendir(src_dir);
    if (!dir) {
        fprintf(stderr, "%d: %s\n\tError opening source directory: %s\n", errno,
                strerror(errno), src_dir);
        return -1;
    }

    struct dirent *entry;
    struct stat src_stat;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir,
                 entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            fprintf(stderr, "%d: %s\n\tCould not stat file (skipping): %s\n",
                    errno, strerror(errno), src_path);
            continue;
        }

        if (S_ISDIR(src_stat.st_mode)) {
            // Recurse into directories
            link_all_files_in_dir(src_path, dest_path);
        } else if (S_ISREG(src_stat.st_mode)) {
            // Create a hard link for regular files
            if (link(src_path, dest_path) == -1) {
                fprintf(stderr,
                        "%d: %s\n\tFailed to create hard link (skipping): %s\n",
                        errno, strerror(errno), dest_path);
            }
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 2 &&
        (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <target_directory>\n",
                argv[0]);
        fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *source_dir = argv[1];
    const char *target_dir = argv[2];
    link_all_files_in_dir(source_dir, target_dir);
    return errno;
}
