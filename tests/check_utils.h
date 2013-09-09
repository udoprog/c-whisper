#ifndef _CHECK_UTILS_H_
#define _CHECK_UTILS_H_

#define CHECK_MAIN(test_f) \
Suite * \
test_suite_main() { \
    Suite *s = suite_create("main"); \
    TCase *tc_core = tcase_create("Core"); \
    tcase_add_test(tc_core, test_f); \
    suite_add_tcase(s, tc_core); \
    return s; \
} \
\
int main() { \
    Suite *s = test_suite_main(); \
    SRunner *sr = srunner_create(s); \
    srunner_run_all(sr, CK_NORMAL); \
    int number_failed = srunner_ntests_failed(sr); \
    srunner_free(sr); \
    return (number_failed == 0) ? 0 : 1; \
}

#endif /* _CHECK_UTILS_H_ */
