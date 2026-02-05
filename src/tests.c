
#define TEST_RUNNER
// #define DEBUG
#include "lndir_uring.c"

int test_string_list() {
    StringList list = StringList_new();
    StringListIter iter = StringListIter_new(&list);

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

    StringList list = StringList_new();
    StringListIter iter = StringListIter_new(&list);

    for (int i = 0; i < 1; i++) {
        StringList_add_nullterm(&list, "a");
        StringList_add_nullterm(&list, "b");
        StringList_add_nullterm(&list, "foo");
        StringList_add_nullterm(&list, "bar");
    }

    hardlink_file_list_iouring(&iter, "tests/basic_in", "tests/basic_out");
}

int main() {
    test_string_list();
    // test_linking_file_list();
}
