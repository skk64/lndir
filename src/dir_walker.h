/*
 * I made this simple file tree walker because the libc nftw doesn't have any way to reference userdata for the callback.
 * This implementation has hardcoded limits on the path length and max directory depth.
*/

#include <dirent.h>
#include <stdlib.h>

#define MAX_PATH_LEN 4096
#define MAX_DEPTH 256

enum simple_ftw_sig {
    S_FTW_CONTINUE = 0,       //
    S_FTW_STOP_ITERATION = 1, // Stops all iteration
    S_FTW_SKIP_DIRECTORY = 2, // If returned on a directory, will skip that directory
};
typedef enum simple_ftw_sig simple_ftw_sig;

/*
 * This is the type of the function that gets called for each directory entry
 * The return value can be used to control the iteration of simple_ftw
*/
typedef simple_ftw_sig (*simple_ftw_callback_t) (
    const struct dirent* dir_entry, // the directory entry of the file
    const char* path,               // the path of the file relative to the path simple_ftw is called with
    unsigned int path_len,          // the length of path
    void* userdata                  // userdata that matches the userdata simple_ftw is called with
);

/*
 * Calls cb for every entry in path.
*/
void simple_ftw(const char* path, simple_ftw_callback_t cb, void* userdata);
