#include "test-codecs.h"

static void*
compress_buffer_thread_func (SquashCodec* codec) {
  const size_t max_compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t compressed_length;
  size_t decompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (max_compressed_length);
  uint8_t* decompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;
  int i = 0;

  for ( ; i < 8 ; i++ ) {
    compressed_length = max_compressed_length;

    res = squash_codec_compress_with_options (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM, NULL);
    SQUASH_ASSERT_OK(res);

    res = squash_codec_decompress_with_options (codec, &decompressed_length, decompressed, compressed_length, compressed, NULL);
    SQUASH_ASSERT_OK(res);
    g_assert (decompressed_length == LOREM_IPSUM_LENGTH);

    g_assert (memcmp (LOREM_IPSUM, decompressed, LOREM_IPSUM_LENGTH) == 0);
  }

  free (compressed);
  free (decompressed);

  return NULL;
}

void
check_codec (SquashCodec* codec) {
  guint n_threads;
  GThread** threads;

#if GLIB_CHECK_VERSION(2, 36, 0)
  n_threads = g_get_num_processors ();
#else
  n_threads = 2;
#endif
  threads = (GThread**) calloc(n_threads, sizeof(GThread*));

  guint i = 0;
  GError* inner_error = NULL;

  for ( ; i < n_threads ; i++ ) {
    threads[i] = g_thread_create ((GThreadFunc) compress_buffer_thread_func, codec, true, &inner_error);
    if (inner_error != NULL) {
      g_error ("Unable to create thread #%d: %s", i, inner_error->message);
    }
  }

  for ( i = 0 ; i < n_threads ; i++ ) {
    g_assert (g_thread_join (threads[i]) == NULL);
  }

  free (threads);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/threads/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
