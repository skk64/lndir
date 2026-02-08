#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

// #define DEBUG

#include "string_list.h"

#define STRINGLIST_START_BLOCK_SIZE 256

#ifndef max
#define max(a, b) a < b ? b : a;
#endif
#ifndef min
#define min(a, b) a < b ? a : b;
#endif

#ifndef debug_printf
#ifdef DEBUG
#define debug_printf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__); 
#else 
#define debug_printf(fmt, ...) 
#endif
#endif

#define next_power_of_2(x) ( x == 0 ? STRINGLIST_START_BLOCK_SIZE : 1 << (sizeof(x) * 8 - __builtin_clz(x - 1)))

// static inline int next_power_of_2(int x) {
//     int clz = __builtin_clz(x - 1);    
//     return 1 << clz;
// }

StringListBlock *StringListBlock_new_blocksize(int blocksize) {
    assert(blocksize > 0);
    debug_printf("new list created, size: %d\n", blocksize);
    StringListBlock *list = calloc(sizeof(StringListBlock) + blocksize, 1);
    if (list == NULL) return NULL;
    list->cap = blocksize;
    return list;
}

StringListBlock *StringListBlock_new() {
    return StringListBlock_new_blocksize(STRINGLIST_START_BLOCK_SIZE);
}

/// Add a string to a string list.
/// Returns a pointer to the latest block
/// Implemented as a singly-linked list of blocks, so the first StringList
/// should be kept and used for iteration
StringListBlock* StringListBlock_add(StringListBlock *list, const char *string, int string_len) {
    if (list == NULL) return NULL;

    debug_printf("List: add string: %s\n", string);

    // Need + 2 so the iterator works as expected
    if (string_len + list->len + 2 >= list->cap) {
        int string_len_next = next_power_of_2(string_len);

        int new_blocksize = max(list->cap * 2, string_len_next);
        StringListBlock *new_list = StringListBlock_new_blocksize(new_blocksize);
        list->next = new_list;
        list = new_list;
    }
    memcpy((char*)list + sizeof(StringListBlock) + list->len, string, string_len );
    list->len += string_len + 1;
    return list;
}

StringListBlock *StringListBlock_add_nullterm(StringListBlock *list, const char *string) {
    unsigned long int len = strlen(string);
    assert(len < INT32_MAX);
    return StringListBlock_add(list, string, (int) len);
}

StringList StringList_new() {
  StringList list;
  list.first = StringListBlock_new();
  list.last = list.first;
  return list;
}


/// Add a string to a string list.
// If the length is unknown, StringList_add_nullterm can be called instead
void StringList_add(StringList *list, const char *string, int string_len) {
    list->last = StringListBlock_add(list->last, string, string_len);
}

/// Add a null-terminated string to a string list.
void StringList_add_nullterm(StringList *list, const char *string) {
    // Initialise if empty
    if (list->first == NULL) *list = StringList_new();
    
    int string_len = strlen(string);
    list->last = StringListBlock_add(list->last, string, string_len);
}

void StringList_free(StringList list) {
    StringListBlock* current = list.first;
    while (current != NULL) {
        StringListBlock* next = current->next;
        free(current);
        current = next;
    }
}


StringListIter StringList_iterate(StringList* list) {
    if (list->first == NULL) *list = StringList_new();
    StringListIter iter;
    iter.list = list->first;
    iter.len = 0;
    return iter;
}


char* StringListIter_next(StringListIter* iter) {
    char* str = (char*)iter->list + sizeof(StringListBlock) + iter->len;
    if (*str == 0) {
        // Try getting next block
        debug_printf("iter: end of block\n");
        if (iter->list->next == NULL) return NULL;
        iter->list = iter->list->next;
        iter->len = 0;
        str = (char*)iter->list + sizeof(StringListBlock);
        assert(str != NULL);
    }
    int len = strlen(str);
    iter->len += len + 1;
    return str;
}


