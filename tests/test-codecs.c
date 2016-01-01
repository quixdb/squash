#include "test-codecs.h"

static void
squash_check_codec_enumerator_codec_cb (SquashCodec* codec, void* data) {
  if (SQUASH_UNLIKELY(squash_codec_init (codec) != SQUASH_OK))
    return;

  squash_check_setup_tests_for_codec (codec, data);
}

static void
squash_check_codec_enumerator_plugin_cb (SquashPlugin* plugin, void* data) {
  if (squash_plugin_init (plugin) != SQUASH_OK) {
    /* If srcdir == builddir then we will pick up *all* of the
       plugins, not just those which were enabled.  This should filter
       out any disabled plugins. */
    return;
  }

  squash_plugin_foreach_codec (plugin, squash_check_codec_enumerator_codec_cb, data);
}

static gchar* squash_plugins_dir = NULL;

static GOptionEntry options[] = {
  { "squash-plugins", 0, 0, G_OPTION_ARG_STRING, &squash_plugins_dir, "Path to the Squash plugins directory", "DIR" },
  { NULL }
};

static void* squash_test_malloc (size_t size) {
  uint64_t* ptr = malloc (size + sizeof(uint64_t));
  *ptr = 0xBADC0FFEE0DDF00D;
  return (void*) (ptr + 1);
}

static void* squash_test_realloc (void* ptr, size_t size) {
  uint64_t* rptr = ((uint64_t*) ptr) - 1;
  g_assert (*rptr == 0xBADC0FFEE0DDF00D);
  rptr = realloc (rptr, size + sizeof(uint64_t));
  return (void*) (rptr + 1);
}

static void squash_test_free (void* ptr) {
  if (ptr == NULL)
    return;

  uint64_t* rptr = ((uint64_t*) ptr) - 1;
  g_assert (*rptr == 0xBADC0FFEE0DDF00D);
  free (rptr);
}

int
main (int argc, char** argv) {
  GError *error = NULL;
  GOptionContext *context;
  SquashMemoryFuncs memfns = {
    squash_test_malloc,
    squash_test_realloc,
    squash_test_free,
    NULL,
  };

  squash_set_memory_functions (memfns);

  g_test_init (&argc, &argv, NULL);

  context = g_option_context_new ("- Squash unit test");
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_set_ignore_unknown_options (context, FALSE);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error ("Option parsing failed: %s\n", error->message);
  }
  g_option_context_free (context);

  if (squash_plugins_dir != NULL) {
    g_setenv ("SQUASH_PLUGINS", squash_plugins_dir, TRUE);
  }

  if (argc > 1) {
    int arg;
    for ( arg = 1 ; arg < argc ; arg++) {
      SquashCodec* codec = squash_get_codec (argv[arg]);
      if (codec != NULL) {
        squash_check_codec_enumerator_codec_cb (codec, NULL);
      } else {
        g_error ("Unable to find requested codec (%s).\n", argv[arg]);
      }
    }
  } else {
    squash_foreach_plugin (squash_check_codec_enumerator_plugin_cb, NULL);
  }

  return g_test_run ();
}
