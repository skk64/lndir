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

/*
 * Add a string to the string list. Createts a copy, so the original string does
 * not need to be retained.
 * If the string length is not known, use StringList_add_nullterm
*/
void StringList_add(StringList *list, const char *string, int string_len);

/*
 * Add a string to the string list. Createts a copy, so the original string does
 * not need to be retained.
 * If the string length is known, use StringList_add. This is just a wrapper
 * that calls strlen followed by StringList_add.
*/
void StringList_add_nullterm(StringList *list, const char *string);

/*
 * Frees all allocated memory in the list.
*/
void StringList_free(StringList list);

/*
 * Returns an iterator that can be used to loop over every string in the list.
*/
StringListIter StringList_iterate(StringList* list);

/*
 * Each call to StringListIter_next returns the next string in the list.
 * Strings are returned in insertion order.
 * Once all strings have been returned, NULL is returned.
*/
char* StringListIter_next(StringListIter* iter);


#endif
