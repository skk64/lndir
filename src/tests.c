
#define TEST_RUNNER
#include "lndir_uring.c"

int test_string_list() {
    StringList* list = StringList_new();
    if (list == NULL) {
      fprintf(stderr, "no list\n");
      return 1;
    }
    StringListIter iter = StringListIter_new(list);

    for (int i = 0; i < 40; i++) {
        list = StringList_add_nullterm(list, "string 1");
        list = StringList_add_nullterm(list, "string 2");
        list = StringList_add_nullterm(list, " 3");
        list = StringList_add_nullterm(list, "string 4");
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

    StringList* list = StringList_new();
    StringListIter iter = StringListIter_new(list);

    for (int i = 0; i < 1; i++) {
        list = StringList_add_nullterm(list, "a");
        list = StringList_add_nullterm(list, "b");
        list = StringList_add_nullterm(list, "foo");
        list = StringList_add_nullterm(list, "bar");
    }

    // hardlink_file_list_iouring(&iter, "tests/basic_in", "tests/basic_out");
}

int main() {
    test_string_list();
    // test_linking_file_list();
}
