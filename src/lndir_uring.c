#define _XOPEN_SOURCE 500 
#define _GNU_SOURCE

// #define DEBUG

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

#include "string_list.h"


#define MAX_SQE 128
// Must be smaller than MAX_SQE
#define SQE_SUBMISSION_SIZE 16

#ifndef debug_printf
#ifdef DEBUG
#define debug_printf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__); 
#else 
#define debug_printf(fmt, ...) 
#endif
#endif


struct LinkResults {
    int successes;
    int total_handled;
};
typedef struct LinkResults LinkResults;

/// For each result in the completion queue, if it was an error, print to stderr
/// 
/// Returns the number of results handled
LinkResults iouring_handle_results(struct io_uring *ring) {
    LinkResults results = {};
    debug_printf("iouring_handle_results:\n");
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
                fprintf(stderr, "%s: %s\n", errmsg, path);
            }
        } else {
            results.successes += 1;
        }
        io_uring_cqe_seen(ring, cqe);
        result = io_uring_peek_cqe(ring, &cqe);
    }
    results.total_handled = count;
    return results;
}

/// For each file in the file list, hard links that file from the source directory to destination directory
/// If any hardlink fails, the result is ignored from the return value of this function
/// However, stderr is printed to.
/// 
/// Returns 0 on success
/// If io_uring fails, returns -errno
/// if the directories are invalid, returns 1
int hardlink_file_list_iouring_fd(StringListIter *file_list, int src_dir_fd, int dest_dir_fd) {
    if (src_dir_fd < 0 || dest_dir_fd < 0) return 1;

    struct io_uring ring;
    int result = io_uring_queue_init(MAX_SQE, &ring, 0);
    if (result != 0)
        return result;

    int submission_count = 0;
    LinkResults counts = {};

    char* file_path;
    while ((file_path = StringListIter_next(file_list)) != NULL) {

        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        // get_sqe returns NULL when the queue is full
        while (sqe == NULL) {
            LinkResults results_handled = iouring_handle_results(&ring);
            counts.total_handled += results_handled.total_handled;
            counts.successes += results_handled.successes;
            sqe = io_uring_get_sqe(&ring);
        }
        io_uring_prep_linkat(sqe, src_dir_fd, file_path, dest_dir_fd, file_path, 0);
        io_uring_sqe_set_data(sqe, file_path);
        submission_count += 1;

        if (submission_count % SQE_SUBMISSION_SIZE == 0) {
            io_uring_submit(&ring);
        }
    }

    io_uring_submit(&ring);

    while (counts.total_handled < submission_count) {
        debug_printf("handled/submitted:   %d/%d\n", counts.total_handled, submission_count);
        // block until ready
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);

        LinkResults results_handled = iouring_handle_results(&ring);
        counts.total_handled += results_handled.total_handled;
        counts.successes += results_handled.successes;
    }
    io_uring_queue_exit(&ring);
    printf("Total linked files:  %d / %d\n", counts.successes, counts.total_handled);
    return 0;
}

/// For each file in the file list, hard links that file from the source directory to destination directory
/// If any hardlink fails, the result is ignored from the return value of this function
/// However, stderr is printed to.
/// 
/// Returns 0 on success
/// If io_uring fails, returns -errno
/// if the directories are invalid, returns 1
int hardlink_file_list_iouring(StringListIter *file_list, const char *src_dir, const char *dest_dir) {
    int src_fd = open(src_dir, O_DIRECTORY);
    int dest_fd = open(dest_dir, O_DIRECTORY);
    int result = hardlink_file_list_iouring_fd(file_list, src_fd, dest_fd);
    close(src_fd);
    close(dest_fd);
    return result;
}


/// These are needed for the nftw callback
// char* source_directory;
// char* destination_directory;
int lndir_source_directory_len = -1;
int lndir_destination_directory_fd = -1;
struct StringList lndir_file_list = {};

/// nftw callback
/// For each directory in source, it creates a matching directory at the destination
/// For each file in source, it adds the relative path to file_list
int copy_directories_add_filenames(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    assert(lndir_source_directory_len > 0);
    assert(lndir_destination_directory_fd > 0);
    const char* file_relative = fpath + lndir_source_directory_len;
    while (file_relative[0] == '/') file_relative += 1;

    switch (sb->st_mode & S_IFMT) {    
    case S_IFREG:
    case S_IFLNK:
        debug_printf("file:             %s\n", fpath);
        StringList_add_nullterm(&lndir_file_list, file_relative);
        break;
    case S_IFDIR:
        debug_printf("dir: %s\n", fpath);
        mkdirat(lndir_destination_directory_fd, file_relative, sb->st_mode);
        break;
    }
    return 0;
}


void hardlink_directory_structure(const char *src_dir, const char *dest_dir) {
    int source_directory_fd = open(src_dir, O_DIRECTORY);
    // Create destination directory
    struct stat src_dir_stat;
    fstat(source_directory_fd, &src_dir_stat);
    mkdir(dest_dir, src_dir_stat.st_mode);

    // need to prepare globals for nftw callback
    lndir_source_directory_len = strlen(src_dir);
    lndir_destination_directory_fd = open(dest_dir, O_DIRECTORY);

    nftw(src_dir, &copy_directories_add_filenames, 128, 0);

    StringListIter iter = StringList_iterate(&lndir_file_list);
    hardlink_file_list_iouring_fd(&iter, source_directory_fd, lndir_destination_directory_fd);

    close(source_directory_fd);
    close(lndir_destination_directory_fd);

    StringList_free(lndir_file_list);
}

