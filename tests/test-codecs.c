#include "test-codecs.h"

static void
squash_check_codec_enumerator_codec_cb (SquashCodec* codec, void* data) {
  gchar* test_name;

  if (squash_codec_init (codec) != SQUASH_OK) {
    return;
  }

  test_name = g_strdup_printf ("/%s/%s/%s",
                               (gchar*) data,
                               squash_plugin_get_name (squash_codec_get_plugin (codec)),
                               squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
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

int
main (int argc, char** argv) {
  GError *error = NULL;
  GOptionContext *context;

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

  gchar* test_name = g_path_get_basename (argv[0]);
  if (strncmp (test_name, "lt-", 3) == 0) {
    gchar* old = test_name;
    test_name = g_strdup (test_name + 3);
    g_free (old);
  }

  if (argc > 1) {
    int arg;
    for ( arg = 1 ; arg < argc ; arg++) {
      SquashCodec* codec = squash_get_codec (argv[arg]);
      if (codec != NULL) {
        squash_check_codec_enumerator_codec_cb (codec, test_name);
      } else {
        g_error ("Unable to find requested codec (%s).\n", argv[arg]);
      }
    }
  } else {
    squash_foreach_plugin (squash_check_codec_enumerator_plugin_cb, test_name);
  }

  g_free (test_name);

  return g_test_run ();
}
