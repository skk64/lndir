#define _XOPEN_SOURCE 500 
#define _GNU_SOURCE

#include <dirent.h>
#include <assert.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <liburing.h>
#include <ftw.h>

#include "string_list.c"


#define MAX_SQE 128
// Must be smaller than MAX_SQE
#define SQE_SUBMISSION_SIZE 16



/// Returns the number of results handled
int iouring_handle_results(struct io_uring *ring) {
    printf("iouring_handle_results:\n");
    struct io_uring_cqe *cqe;
    int result;
    int count = 0;
    result = io_uring_peek_cqe(ring, &cqe);
    while (result == 0) {
        count += 1;
        char *path = io_uring_cqe_get_data(cqe);
        if (cqe->res < 0) {
            char *errmsg = strerror(-cqe->res);
            if (errmsg != NULL) {
                fprintf(stderr, "%s: Error linking %s\n", errmsg, path);
            }
        }
        io_uring_cqe_seen(ring, cqe);
        result = io_uring_peek_cqe(ring, &cqe);
    }
    return count;
}

int hardlink_file_list_iouring(StringListIter *file_list, const char *src_dir, const char *dest_dir) {
    if (src_dir == NULL || dest_dir == NULL) return 1;
    int src_dir_len = strlen(src_dir);
    // int dest_dir_len = strlen(dest_dir);

    struct io_uring ring;
    int result = io_uring_queue_init(MAX_SQE, &ring, 0);
    if (result != 0)
        return result;


    int src_fd = open(src_dir, O_DIRECTORY);
    int dest_fd = open(dest_dir, O_DIRECTORY);

    int submission_count = 0;
    int handled_count = 0;

    char* file;
    while ((file = StringListIter_next(file_list)) != NULL) {
        printf("link file: %s\n", file);
        char* file_relative = file + src_dir_len;
        while (file_relative[0] == '/') file_relative += 1;

        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        // get_sqe returns NULL when the queue is full
        while (sqe == NULL) {
            int results_handled = iouring_handle_results(&ring);
            handled_count += results_handled;
            sqe = io_uring_get_sqe(&ring);
        }
        io_uring_prep_linkat(sqe, src_fd, file_relative, dest_fd, file_relative, 0);
        io_uring_sqe_set_data(sqe, file);
        submission_count += 1;

        if (submission_count % SQE_SUBMISSION_SIZE == 0) {
            io_uring_submit(&ring);
        }
    }

    io_uring_submit(&ring);

    while (handled_count < submission_count) {
        printf("handled/submitted:   %d/%d\n", handled_count, submission_count);
        // block until ready
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);

        int results_handled = iouring_handle_results(&ring);
        handled_count += results_handled;
    }
    printf("handled/submitted:   %d/%d\n", handled_count, submission_count);
    return 0;
}




struct StringList file_list;

int directory_entry_handler(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    switch (sb->st_mode & S_IFMT) {    
    case S_IFREG:
    case S_IFLNK:
        // printf("file: %s\n", fpath);
        StringList_add_nullterm(&file_list, fpath);
        break;
    case S_IFDIR:
        printf("dir: %s\n", fpath);
        break;
    }
    return 0;
}


/// Uses io_uring to asynchronously link every file
int hardlink_directory_iouring(const char *src_dir, const char *dest_dir) {
    return 0;
}


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


#ifndef TEST_RUNNER
int main(int argc, char *argv[]) {
    file_list = StringList_new();

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

    nftw(input, &directory_entry_handler, 128, 0);

    StringListIter iter = StringListIter_new(&file_list);
    hardlink_file_list_iouring(&iter, input, output);

    // iter = StringListIter_new(&file_list);
    // char* filename;
    // printf("\n\n");
    // while ( (filename = StringListIter_next(&iter)) != NULL) {
    //     printf("file: %s\n", filename);
    // }

    StringList_free(file_list);
}
#endif
