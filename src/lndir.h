
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


/*
 * Duplicates the directory structure in src_dir to dest_dir,
 * then hardlinks every file in src_dir to a matching file in dest_dir.
*/
enum lndir_result hardlink_directory_structure(const char* src_dir, const char* dest_dir);

#endif
