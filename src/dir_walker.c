#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "debug.h"
#include "dir_walker.h"


static bool is_valid_path(const char* path) {
    return strcmp(".", path) != 0 && strcmp("..", path) != 0;
}


/// Returns the new length, or 0 if it fails because the MAX_PATH_LEN is reached
unsigned int append_path(char* source_path, unsigned int source_path_len, const char* path_to_append) {
    int extra_len = strlen(path_to_append);
    if (source_path_len + extra_len + 2 >= MAX_PATH_LEN) return 0;

    if (source_path_len == 0) {
        memcpy(source_path, path_to_append, extra_len);
        return extra_len;
    }

    if (source_path[source_path_len - 1] != '/') {
        source_path[source_path_len] = '/';
        source_path_len += 1;
    }
    memcpy(source_path + source_path_len, path_to_append, extra_len);
    int new_len = source_path_len + extra_len;
    source_path[new_len] = 0;
    return new_len;
}

static simple_ftw_sig recurse_simple_ftw(char* path, unsigned int path_len, int depth, simple_ftw_callback_t cb, void* userdata) {
    if (depth >= MAX_DEPTH) return S_FTW_CONTINUE;
    path[path_len] = 0;

    DIR* dir = opendir(path);
    if (dir == NULL) return S_FTW_CONTINUE;

    debug_printf("ftw start: '%s'\n", path);
    struct dirent* ent;

    while ((ent = readdir(dir)) != NULL) {
        int new_len = append_path(path, path_len, ent->d_name);
        debug_printf("ftw path: '%s' / '%s'\n", path, ent->d_name);
        if (new_len <= path_len) continue;
        if (!is_valid_path(ent->d_name)) continue;

        if (cb) {
            switch (cb(ent, path, new_len, userdata)) {
                case S_FTW_STOP_ITERATION:
                    closedir(dir);
                    return S_FTW_STOP_ITERATION;
                case S_FTW_SKIP_DIRECTORY:
                    continue;
                case S_FTW_CONTINUE:
                    {}
            }
        } 

        if (ent->d_type == DT_DIR) {
            simple_ftw_sig result = recurse_simple_ftw(path, new_len, depth + 1, cb, userdata);
            if (result == S_FTW_STOP_ITERATION) {
                closedir(dir);
                return S_FTW_STOP_ITERATION;
            }
        }
    }
    closedir(dir);
    return S_FTW_CONTINUE;
}



void simple_ftw(const char* path, simple_ftw_callback_t cb, void* userdata) {
    char buf[MAX_PATH_LEN];
    int path_len = strlen(path);
    if (path_len >= MAX_PATH_LEN) return;
    memcpy(buf, path, path_len);
    recurse_simple_ftw(buf, path_len, 1, cb, userdata);
}
