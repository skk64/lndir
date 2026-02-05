#include <dirent.h>
// #include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <liburing.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct StringList {
    struct StringList *next;
    char *data;
    int len;
};
typedef struct StringList StringList;



#define STRINGLIST_BLOCK_SIZE 4096
#define MAX_SQE 128
// Must be smaller than MAX_SQE
#define SQE_SUBMISSION_SIZE 16

StringList *StringList_new() {
    // printf("new list called\n");
    StringList *list = calloc(1, sizeof(StringList));
    if (list == NULL) return NULL;
    list->data = calloc(STRINGLIST_BLOCK_SIZE, sizeof(char));
    if (list->data == NULL) {
        free(list);
        return NULL;
    }
    return list;
} 

/// Add a string to a string list.
/// Returns a pointer to the latest block
/// Implemented as a singly-linked list of blocks, so the first StringList
/// should be kept and used for iteration
StringList *StringList_add(StringList *list, char *string, int string_len) {
    if (list == NULL) return NULL;
    if (string_len > STRINGLIST_BLOCK_SIZE) return NULL;

    if (string_len + list->len >= STRINGLIST_BLOCK_SIZE) {
        StringList *new_list = StringList_new();
        list->next = new_list;
        list = new_list;
    }
    memcpy(list->data + list->len, string, string_len);
    list->len += string_len + 1;
    return list;
}

StringList *StringList_add_nullterm(StringList *list, char *string) {
    int len = strlen(string);
    StringList_add(list, string, len);
}



struct StringListIter {
    struct StringList list;
};
typedef struct StringListIter StringListIter;

StringListIter StringListIter_new(StringList* list) {
    StringListIter iter;
    iter.list = *list;
    iter.list.len = 0;
    return iter;
}


/// Returns the next string in a string list block. Returns NULL if it is at the end of the block
char* StringList_next_string(StringList *list) {
    char *start = list->data + list->len;
    if (*start == 0) {
        return NULL;
    } else {
        int len = strlen(start);
        list->len += len + 1;
        return start;
    }
}

char* StringListIter_next(StringListIter* iter) {
    char* str = StringList_next_string(&iter->list);
    int i = 0;
    while (str == NULL) {
        assert(i++ < 3 && "StringListIterNext looping"); // Almost certainly something wrong if this triggers
        // Push to next block
        if (iter->list.next == NULL) {
            return NULL;
        } else {
            StringList* next = iter->list.next;
            iter->list.len = 0;
            iter->list.data = next->data;
            iter->list.next = next->next;
            str = StringList_next_string(&iter->list);
        }
    }
    return str;
}


/// Returns the number of results handled
int iouring_handle_results(struct io_uring *ring) {
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
    if (src_dir == NULL || dest_dir == NULL)
        return 1;

    struct io_uring ring;
    int result = io_uring_queue_init(MAX_SQE, &ring, 0);
    if (result != 0)
        return result;

    int src_fd = open(src_dir, O_DIRECTORY);
    int dest_fd = open(dest_dir, O_DIRECTORY);


    int submission_count = 0;
    int handled_count = 0;
    char* file = StringListIter_next(file_list);

    while (file_list != NULL) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        // get_sqe returns NULL when the queue is full
        while (sqe == NULL) {
            int results_handled = iouring_handle_results(&ring);
            handled_count += results_handled;
            sqe = io_uring_get_sqe(&ring);
        }
        io_uring_prep_linkat(sqe, src_fd, file, dest_fd, file, 0);
        io_uring_sqe_set_data(sqe, file);
        submission_count += 1;

        if (submission_count % SQE_SUBMISSION_SIZE == 0) {
            result = io_uring_submit(&ring);
        }
        file = StringListIter_next(file_list);
    }

    while (handled_count < submission_count) {
        // block until ready
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);

        int results_handled = iouring_handle_results(&ring);
        handled_count += results_handled;
    }
    return 0;
}

/// Uses io_uring to asynchronously link every file
int hardlink_directory_iouring(const char *src_dir, const char *dest_dir) {
    return 0;
}

#ifndef TEST_RUNNER
int main() {
    StringList* list = StringList_new();
    StringList* first_list = list;
}
#endif
