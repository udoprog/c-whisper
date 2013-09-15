#include <check.h>

#include "../src/wsp.h"
#include "../src/wsp_memfs.h"

#include "check_utils.h"

wsp_mapping_t m = WSP_MEMORY;
wsp_aggregation_t a = WSP_AVERAGE;
float xff = 0.5;

void setup()
{
    wsp_archive_input_t archives[] = {
        { .spp = 10, .count = 100 },
        { .spp = 20, .count = 100 },
        { .spp = 40, .count = 100 }
    };

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    ck_assert_int_eq(
        WSP_OK, wsp_create("a1", archives, 3, a, xff, m, &e)
    );

    wsp_ck_file_size("a1", archives, 3);
}

void teardown()
{
}

START_TEST(test_update_aggregation_1)
{
    wsp_t w;
    WSP_INIT(&w);

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    wsp_return_t r;

    r = wsp_open(&w, "a1", m, WSP_READ | WSP_WRITE, &e);
    ck_assert_msg(r==WSP_OK, wsp_strerror(&e));

    wsp_point_input_t input1 = { .timestamp = 10, .value = 1.0 };
    r = wsp_update_now(&w, &input1, 20, &e);
    ck_assert_msg(r==WSP_OK, wsp_strerror(&e));

    wsp_point_input_t input2 = { .timestamp = 20, .value = 1.0 };
    r = wsp_update_now(&w, &input2, 20, &e);
    ck_assert_msg(r==WSP_OK, wsp_strerror(&e));

    wsp_point_t p1[2];
    uint32_t s1;

    wsp_point_t p2[2];
    uint32_t s2;

    r = wsp_fetch_time_points(&w, w.archives, 10, 20, p1, &s1, &e);
    ck_assert_msg(r==WSP_OK, wsp_strerror(&e));
    ck_assert_int_eq(s1, 2);

    r = wsp_fetch_time_points(&w, w.archives + 1, 20, 20, p2, &s2, &e);
    ck_assert_msg(r==WSP_OK, wsp_strerror(&e));
    ck_assert_int_eq(s2, 1);

    ck_assert(p1[0].timestamp == 10 && p1[0].value == 1.0);
    ck_assert(p1[1].timestamp == 20 && p1[1].value == 1.0);

    ck_assert(p2[0].timestamp == 20 && p2[0].value == 1.0);
}
END_TEST

Suite *
test_suite_main() {
    Suite *s = suite_create("main");
    TCase *tc_core = tcase_create("Whisper update");

    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_update_aggregation_1);

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
