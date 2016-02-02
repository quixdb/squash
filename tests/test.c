#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 200112L)
#  undef _POSIX_C_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200112L
#endif

#include "test-squash.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#  define snprintf _snprintf
#endif
#define SQUASH_PTR_TEST_INT INT64_C(0xBADC0FFEE0DDF00D)

static void* squash_test_malloc (size_t size) {
  uint64_t* ptr = malloc (size + sizeof(uint64_t));
  *ptr = SQUASH_PTR_TEST_INT;
  return (void*) (ptr + 1);
}

static void* squash_test_calloc (size_t nmemb, size_t size) {
  uint64_t* ptr = calloc (1, (nmemb * size) + sizeof(uint64_t));
  *ptr = SQUASH_PTR_TEST_INT;
  return (void*) (ptr + 1);
}

static void* squash_test_realloc (void* ptr, size_t size) {
  if (ptr == NULL) {
    return squash_test_malloc (size);
  } else {
    uint64_t* rptr = ((uint64_t*) ptr) - 1;
    munit_assert_cmp_uint64 (*rptr, ==, SQUASH_PTR_TEST_INT);
    rptr = realloc (rptr, size + sizeof(uint64_t));
    return (void*) (rptr + 1);
  }
}

static void squash_test_free (void* ptr) {
  if (ptr == NULL)
    return;

  uint64_t* rptr = ((uint64_t*) ptr) - 1;
  munit_assert_cmp_uint64 (*rptr, ==, SQUASH_PTR_TEST_INT);
  free (rptr);
}

void*
squash_test_get_codec(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  return squash_get_codec (munit_parameters_get (params, "codec"));
}

static size_t codec_list_l = 0;

MunitParameterEnum* squash_codec_parameter = (MunitParameterEnum[]) {
  { (char*) "codec", NULL },
  { NULL, NULL }
};

static void
squash_codec_generate_list (SquashCodec* codec, void* user_data) {
  SquashPlugin* plugin = squash_codec_get_plugin (codec);
  if (squash_plugin_init (plugin) != SQUASH_OK) {
    /* If srcdir == builddir then we will pick up *all* of the
       plugins, not just those which were enabled.  This should filter
       out any disabled plugins. */
    return;
  }

  const char* plugin_name = squash_plugin_get_name (plugin);
  const char* codec_name = squash_codec_get_name (codec);
  size_t l = strlen (plugin_name) + 1 + strlen(codec_name) + 1;
  char* full_name = malloc(l + 1);
  snprintf(full_name, l, "%s:%s", plugin_name, codec_name);

  squash_codec_parameter->values = realloc(squash_codec_parameter->values, sizeof(char*) * (codec_list_l + 2));
  squash_codec_parameter->values[codec_list_l++] = full_name;
  squash_codec_parameter->values[codec_list_l] = NULL;
}

static void
squash_suite_set_params (MunitSuite* suite) {
  for (MunitTest* test = suite->tests ; test != NULL && test->test != NULL ; test++)
    if (test->parameters == SQUASH_CODEC_PARAMETER) {
      test->parameters = squash_codec_parameter;
      if (test->setup == NULL)
        test->setup = squash_test_get_codec;
    }

  for (MunitSuite* child_suite = suite->suites ; child_suite != NULL && child_suite->prefix != NULL ; child_suite++)
    squash_suite_set_params (child_suite);
}

int
main(int argc, char* const argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  MunitSuite test_suites[] = {
    squash_test_suite_buffer,
    squash_test_suite_bounds,
    squash_test_suite_file,
    squash_test_suite_flush,
    squash_test_suite_random,
    squash_test_suite_splice,
    squash_test_suite_stream,
    squash_test_suite_threads,
    { NULL, NULL, 0, 0 }
  };
  MunitSuite squash_suite = {
    NULL,
    NULL,
    test_suites,
    1,
    MUNIT_SUITE_OPTION_NONE
  };
  SquashMemoryFuncs memfns = {
    squash_test_malloc,
    squash_test_realloc,
    squash_test_calloc,
    squash_test_free,
    NULL, NULL
  };

  squash_set_memory_functions (memfns);

  if (getenv ("SQUASH_PLUGINS") == NULL) {
#if !defined(_WIN32)
    setenv("SQUASH_PLUGINS", SQUASH_TEST_PLUGIN_DIR, 1);
#else
    _putenv_s("SQUASH_PLUGINS", SQUASH_TEST_PLUGIN_DIR);
#endif
  }

  squash_foreach_codec (squash_codec_generate_list, NULL);
  if (codec_list_l == 0) {
    fprintf(stderr, "Unable to find any plugins in `%s'.\n", getenv ("SQUASH_PLUGINS"));
    return EXIT_FAILURE;
  }

  squash_suite_set_params (&squash_suite);

  int ret = munit_suite_main(&squash_suite, NULL, argc, argv);

  for (char** c = (char**) squash_codec_parameter->values ; c != NULL && *c != NULL ; c++)
    free(*c);
  free(squash_codec_parameter->values);

  return ret;
}
