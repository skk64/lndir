
#ifndef LNDIR_H
#define LNDIR_H

#include "string_list.h"

enum lndir_result {
    LNDIR_SUCCESS = 0,
    LNDIR_SRC_OPEN = 1,
    LNDIR_SRC_STAT = 2,
    LNDIR_DEST_CREATE = 3,
    LNDIR_DEST_OPEN = 4,
    LNDIR_IO_URING = 5,
};

typedef int (*lndir_callback_t)(char* path, int result, void* userdata);

/*
 * Duplicates the directory structure in src_dir to dest_dir,
 * then hardlinks every file in src_dir to a matching file in dest_dir.
 *
 * If successes and total are not NULL,
 * the number of files succesfully linked, and total attempted links are written to them respectively.
 *
 * If cb is not NULL, it is called for each link result
 *
 *  Returns:
 *   0 on success
 *   1 if source directory couldn't be opened
 *   2 if source directory couldn't be stat-ed
 *   3 if destination directory couldn't be created
 *   4 if destination directory couldn't be opened
 *   5 if io_uring fails
 *
 *   For any non-zero return, errno is set to the reason for the failure
 */
enum lndir_result hardlink_directory_structure(
    const char* src_dir, const char* dest_dir, lndir_callback_t cb, void* userdata);

#endif
