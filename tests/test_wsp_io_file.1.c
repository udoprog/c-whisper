#include <check.h>
#include "check_utils.h"
#include <stdlib.h>

#include "../src/wsp_io_file.h"

int testcase = 0;


long ref_offset = 0;
long not_offset = -1;
size_t ref_size = 1000;
char malloc_b2[1000];
char malloc_b3[1000];
void *ref_fd = (void *)0xff00ff00;

int malloc_called;
int ftell_called;
int free_called;

void setup_calls() {
    malloc_called = 0;
    ftell_called = 0;
    free_called = 0;
}

void teardown_calls() {
    ck_assert_int_eq(malloc_called, 0);
    ck_assert_int_eq(ftell_called, 0);
    ck_assert_int_eq(free_called, 0);
}

START_TEST(test_failed_malloc)
{
    testcase = 1;
    wsp_error_t e;
    void *buf = NULL;

    int ret = wsp_io_file.read(NULL, 0, 0, &buf, &e);

    ck_assert_int_eq(ret, WSP_ERROR);
    ck_assert_int_eq(e.type, WSP_ERROR_MALLOC);

    malloc_called -= 1;
}
END_TEST

START_TEST(test_failed_ftell)
{
    testcase = 2;
    wsp_error_t e;
    wsp_t w = {
        .io_fd = ref_fd
    };
    void *buf = NULL;

    int ret = wsp_io_file.read(&w, ref_offset, ref_size, &buf, &e);

    ck_assert_int_eq(ret, WSP_ERROR);
    ck_assert_int_eq(e.type, WSP_ERROR_OFFSET);

    malloc_called -= 1;
    ftell_called -= 1;
    free_called -= 1;
}
END_TEST

Suite *
test_suite_main() {
    Suite *s = suite_create("wsp_io_file");

    TCase *file_read = tcase_create("file read");

    tcase_add_checked_fixture(file_read, setup_calls, teardown_calls);
    tcase_add_test(file_read, test_failed_malloc);
    tcase_add_test(file_read, test_failed_ftell);

    suite_add_tcase(s, file_read);
    return s;
}

int main() {
    Suite *s = test_suite_main();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}

void *__malloc(size_t size) {
    malloc_called += 1;

    if (testcase == 1) {
        return NULL;
    }

    if (testcase == 2) {
        ck_assert(size == ref_size);
        return malloc_b2;
    }

    if (testcase == 3) {
        ck_assert(size == ref_size);
        return malloc_b3;
    }

    ck_abort();
    return NULL;
}

long __ftell(FILE *fd) {
    ftell_called += 1;

    if (testcase == 2) {
        ck_assert(fd == ref_fd);
        return not_offset;
    }

    if (testcase == 3) {
        ck_assert(fd == ref_fd);
        return not_offset;
    }

    ck_abort();
    return 0;
}

void __free(void *buffer) {
    free_called += 1;

    if (testcase == 2) {
        ck_assert(buffer == malloc_b2);
        return;
    }

    ck_abort();
}

#define malloc(s) __malloc(s)
#define ftell(s) __ftell(s)
#define free(s) __free(s)
#include "../src/wsp_io_file.c"
#undef malloc
#undef ftell
#undef free
