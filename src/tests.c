
#define TEST_RUNNER
// #define DEBUG
#include "lndir_uring.c"
#include "string_list.h"


#define expectEqualStrings(a, b) { int i = strcmp(a, b); if (i != 0) { printf("MISMATCH: %s  /  %s\n", a, b); return 1; } }

#define strings_len 16

int test_string_list_2() {

    char* strings[strings_len] = { 
        "various/tests/foo/z/c",
        "various/tests/foo/z/x/asdf",
        "various/tests/foo/b",
        "various/tests/foo/a",
        "various/tests/basic_in/b",
        "various/tests/basic_in/foo",
        "various/tests/basic_in/bar",
        "various/tests/basic_out/a",
        "various/tests/basic_out/b",
        "various/tests/basic_out/foo",
        "various/tests/basic_out/bar",
        "various/tests/foo_out/z/x/asdf",
        "various/tests/foo_out/z/c",
        "various/tests/foo_out/b",
        "various/tests/foo_out/a",
        "various/README.md",
    };

    StringList list = {};
    for (int i = 0; i < strings_len; i++) StringList_add_nullterm(&list, strings[i]);

    printf("checking\n");

    StringListIter iter = StringList_iterate(&list);
    for (int i = 0; i < strings_len; i++) {

// #define expectEqualStrings(a, b) { int i = strcmp(a, b); if (i != 0) { printf("MISMATCH: %s  /  %s\n", a, b); return 1; } }
        char* a = StringListIter_next(&iter); 
        char* b = strings[i];
        int i = strcmp(a, b);
        if (i != 0) {
            printf("MISMATCH: %s  ||  %s\n", a, b);
            return 1;
        } else {
            printf("%s\n", a);
        }
    }
    return 0;

}




int test_string_list() {
    StringList list = {};
    StringListIter iter = StringList_iterate(&list);

    for (int i = 0; i < 40; i++) {
        StringList_add_nullterm(&list, "string 1");
        StringList_add_nullterm(&list, "string 2");
        StringList_add_nullterm(&list, " 3");
        StringList_add_nullterm(&list, "string 4");
    }

    char* out = StringListIter_next(&iter);
    int i = 0;

    while (out != NULL) {
        printf("string: '%s' (%d)\n", out, iter.len);
        out = StringListIter_next(&iter);
        // if (i++ > 10) break;
    }
    return 0;
}


int test_linking_file_list() {
    StringList list = {};
    StringListIter iter = StringList_iterate(&list);

    for (int i = 0; i < 1; i++) {
        StringList_add_nullterm(&list, "a");
        StringList_add_nullterm(&list, "b");
        StringList_add_nullterm(&list, "foo");
        StringList_add_nullterm(&list, "bar");
    }

    hardlink_file_list_iouring(&iter, "tests/basic_in", "tests/basic_out");
}

int main() {
    // test_string_list();
    test_string_list_2();
    // test_linking_file_list();
}
