
#ifndef LNDIR_H
#define LNDIR_H

#include "string_list.h"

/*
 * Duplicates the directory structure in src_dir to dest_dir,
 * then hardlinks every file in src_dir to a matching file in dest_dir.
*/
void hardlink_directory_structure(const char* src_dir, const char* dest_dir);

#endif
