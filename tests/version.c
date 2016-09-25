#include "test-squash.h"

static MunitResult
squash_test_version(MUNIT_UNUSED const MunitParameter params[], void* user_data) {

    SQUASH_ASSERT_OK( squash_version() == 8000 );

    munit_assert_string_equal( squash_version_api(), "0.8" );

    return MUNIT_OK;
}

MunitTest squash_version_tests[] = {
    { (char*) "/version", squash_test_version, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_version = {
    (char*) "",
    squash_version_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE
};
