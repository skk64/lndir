/**
 *  A String Collection Data Structure.
 *
 * When strings are added to the list, they are interned into a block.
 * Multiple blocks are connected as a singly-linked list.
 * 
 * This is designed for fast insertion and iteration and not much else.
*/

/** Example Usage
// Lists can be zero-initiased, e.g.:
StringList list = {};

// Then they can be appended to:
StringList_add(&list, str, str_len);
StringList_add_nullterm(&list, "string");

// Iterate over the list by constructing an iterator and calling StringListIter_next:
StringListIter iter = StringList_iterate(&list);

while ((str = StringListIter_next(&iter)) != NULL) {
    printf("%s\n", str);
}

*/


#ifndef STRING_LIST_H
#define STRING_LIST_H


struct StringListBlock {
    struct StringListBlock *next;
    int len;
    int cap;
};
typedef struct StringListBlock StringListBlock;


struct StringList {
    StringListBlock* first;
    StringListBlock* last;
};
typedef struct StringList StringList;


struct StringListIter {
    struct StringListBlock* list;
    int len;
};
typedef struct StringListIter StringListIter;



void StringList_add(StringList *list, const char *string, int string_len);
void StringList_add_nullterm(StringList *list, const char *string);
void StringList_free(StringList list);
StringListIter StringList_iterate(StringList* list);

char* StringListIter_next(StringListIter* iter);


#endif
