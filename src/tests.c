
#define TEST_RUNNER
#include "lndir_uring.c"

int test_string_list() {
    StringList* list = StringList_new();
    StringListIter iter = StringListIter_new(list);

    for (int i = 0; i < 4; i++) {
        list = StringList_add_nullterm(list, "string 1");
        list = StringList_add_nullterm(list, "string 2");
        list = StringList_add_nullterm(list, " 3");
        list = StringList_add_nullterm(list, "string 4");
    }

    char* out = StringListIter_next(&iter);
    int i = 0;

    while (out != NULL) {
        printf("string: '%s' (%d)\n", out, iter.list.len);
        out = StringListIter_next(&iter);
        // if (i++ > 10) break;
    }
    return 0;
}

int main() {
    test_string_list();
}
