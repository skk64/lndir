#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <assert.h>
#include <ftw.h>
#include <liburing.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "lndir.h"
#include "dir_walker.h"

// Size of the Submission Queue
#define MAX_SQE 128
// Number of submissions to queue before submitting
// Must be smaller than MAX_SQE
#define SQE_SUBMISSION_SIZE 64

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
LinkResults iouring_handle_results(struct io_uring* ring) {
    LinkResults results = {};
    debug_printf("iouring_handle_results:\n");
    struct io_uring_cqe* cqe;
    int result;
    int count = 0;
    result = io_uring_peek_cqe(ring, &cqe);
    while (result == 0) {
        count += 1;
        char* path = io_uring_cqe_get_data(cqe);
        if (cqe->res < 0) {
            char* errmsg = strerror(-cqe->res);
            if (errmsg != NULL) fprintf(stderr, "%s: %s\n", errmsg, path);
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
/// If io_uring fails, returns errno
int hardlink_file_list_iouring_fd(StringListIter* file_list, int src_dir_fd, int dest_dir_fd) {
    assert(src_dir_fd > 0);
    assert(dest_dir_fd > 0);

    struct io_uring ring;
    int result = io_uring_queue_init(MAX_SQE, &ring, 0);
    if (result != 0) return -result;

    int submission_count = 0;
    LinkResults counts = {};

    char* file_path;
    while ((file_path = StringListIter_next(file_list)) != NULL) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
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
        struct io_uring_cqe* cqe;
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
int hardlink_file_list_iouring(StringListIter* file_list, const char* src_dir, const char* dest_dir) {
    int src_fd = open(src_dir, O_DIRECTORY);
    int dest_fd = open(dest_dir, O_DIRECTORY);
    int result = hardlink_file_list_iouring_fd(file_list, src_fd, dest_fd);
    close(src_fd);
    close(dest_fd);
    return result;
}

struct WalkerContext {
    struct StringList file_list;
    int source_directory_len;
    int destination_directory_fd;
};
typedef struct WalkerContext WalkerContext;

/// nftw callback
/// For each directory in source, it creates a matching directory at the destination
/// For each file in source, it adds the relative path to file_list
simple_ftw_sig copy_directories_add_filenames(const struct dirent* dir_entry, const char* path, unsigned int path_len, void* userdata) {
    WalkerContext* ctx = (WalkerContext*)userdata;

    assert(ctx->source_directory_len > 0);
    assert(ctx->destination_directory_fd > 0);
    const char* file_relative = path + ctx->source_directory_len;
    while (file_relative[0] == '/') file_relative += 1;

    struct stat st;

    switch (dir_entry->d_type) {
        case DT_DIR:
            stat(path, &st);
            debug_printf("dir: %s\n", path);
            mkdirat(ctx->destination_directory_fd, file_relative, st.st_mode);
            break;
        case DT_REG:
            debug_printf("file: %s\n", path);
            StringList_add_nullterm(&ctx->file_list, file_relative);
            break;
    }
           
    return S_FTW_CONTINUE;
}


enum lndir_result hardlink_directory_structure(const char* src_dir, const char* dest_dir) {
    int result = 0;
    int source_directory_fd = open(src_dir, O_DIRECTORY);
    if (source_directory_fd == -1) goto cleanup_1;

    // Create destination directory
    struct stat src_dir_stat;
    result = fstat(source_directory_fd, &src_dir_stat);
    if (result == -1) goto cleanup_2;
    int dir_result = mkdir(dest_dir, src_dir_stat.st_mode);
    if (dir_result == -1 && errno != EEXIST)  goto cleanup_3; 

    WalkerContext ctx = {0};
    ctx.source_directory_len = strlen(src_dir);
    ctx.destination_directory_fd = open(dest_dir, O_DIRECTORY);
    if (ctx.destination_directory_fd == -1) goto cleanup_4;

    simple_ftw(src_dir, &copy_directories_add_filenames, &ctx);

    StringListIter iter = StringList_iterate(&ctx.file_list);
    errno = hardlink_file_list_iouring_fd(&iter, source_directory_fd, ctx.destination_directory_fd);
    if (errno != 0) goto cleanup_5; 

    result = -5;
cleanup_5:
    result += 1;
    StringList_free(&ctx.file_list);
    close(ctx.destination_directory_fd);
cleanup_4:
    result += 1;
cleanup_3:
    result += 1;
cleanup_2:
    result += 1;
    close(source_directory_fd);
cleanup_1:
    result += 1;
    return result;
}
