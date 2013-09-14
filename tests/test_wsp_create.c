#include <check.h>

#include "../src/wsp.h"
#include "../src/wsp_memfs.h"

#include "check_utils.h"

wsp_mapping_t m = WSP_MEMORY;
wsp_aggregation_t a = WSP_AVERAGE;
float xff = 0.5;

START_TEST(test_empty_archive_1)
{
    wsp_archive_input_t archives[] = {
        { .spp = 1, .count = 0 }
    };

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    ck_assert_int_eq(
        WSP_ERROR, wsp_create("a1", archives, 1, a, xff, m, &e)
    );

    ck_assert_int_eq(WSP_ERROR_ARCHIVE, e.type);
    wsp_ck_not_file("a2");
}
END_TEST

START_TEST(test_empty_archive_2)
{
    wsp_archive_input_t archives[] = {
        { .spp = 0, .count = 1 }
    };

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    ck_assert_int_eq(
        WSP_ERROR, wsp_create("a2", archives, 1, a, xff, m, &e)
    );

    ck_assert_int_eq(WSP_ERROR_ARCHIVE, e.type);
    wsp_ck_not_file("a2");
}
END_TEST

START_TEST(test_write_and_read_back)
{
    wsp_archive_input_t archives[] = {
        { .spp = 60, .count = 10 },
        { .spp = 120, .count = 6 }
    };

    size_t archive_count = sizeof(archives) / sizeof(wsp_archive_input_t);

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    ck_assert_int_eq(
        WSP_OK, wsp_create("a3", archives, archive_count, a, xff, m, &e)
    );

    wsp_ck_file_size("a3", archives, archive_count);
}
END_TEST

START_TEST(test_decreasing_retention)
{
    wsp_archive_input_t archives[] = {
        { .spp = 60, .count = 10 },
        { .spp = 120, .count = 5 }
    };

    size_t archive_count = sizeof(archives) / sizeof(wsp_archive_input_t);

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    ck_assert_int_eq(
        WSP_ERROR, wsp_create("a4", archives, archive_count, a, xff, m, &e)
    );

    ck_assert_int_eq(WSP_ERROR_ARCHIVE, e.type);
    wsp_ck_not_file("a4");
}
END_TEST

Suite *
test_suite_main() {
    Suite *s = suite_create("main");
    TCase *tc_core = tcase_create("Invalid archives should cause errors");

    tcase_add_test(tc_core, test_empty_archive_1);
    tcase_add_test(tc_core, test_empty_archive_2);
    tcase_add_test(tc_core, test_write_and_read_back);
    tcase_add_test(tc_core, test_decreasing_retention);

    suite_add_tcase(s, tc_core);
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
