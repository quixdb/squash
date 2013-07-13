#include <check.h>
#include <stdlib.h>

#include <stdio.h>

#include <squash/squash.h>

#include "squash-bzip2.h"

Suite* options_suite (void);

START_TEST (test_option_parsing) {
  SquashPlugin* plugin = NULL;
  SquashCodec* codec = NULL;
  SquashBZ2Options* options = NULL;

  plugin = squash_get_plugin ("bzip2");
  ck_assert (plugin != NULL);

  codec = squash_plugin_get_codec (plugin, "bzip2");
  ck_assert (codec != NULL);

  options = (SquashBZ2Options*) squash_options_new (codec, "level", "9", "work-factor", "30", NULL);
  ck_assert (options != NULL);
  ck_assert (options->block_size_100k == 9);
  ck_assert (options->work_factor = 30);
  squash_object_unref (options);
} END_TEST

Suite* options_suite (void) {
  Suite *s = suite_create ("Options");

  TCase *tc_option_parsing = tcase_create ("Option Parsing");
  tcase_add_test (tc_option_parsing, test_option_parsing);
  suite_add_tcase (s, tc_option_parsing);

  return s;
}

int
main (int argc, char** argv) {
  int number_failed;
  Suite *s = options_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
