#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  const size_t max_uncompressed_length = 4096;
  size_t uncompressed_length;
  uint8_t uncompressed_data[max_uncompressed_length];
  size_t compressed_length;
  uint8_t* compressed_data = (uint8_t*) malloc (squash_codec_get_max_compressed_size (codec, max_uncompressed_length));
  size_t decompressed_length;
  uint8_t* decompressed_data = (uint8_t*) malloc (max_uncompressed_length);

  size_t uncompressed_data_filled = 0;
  SquashStatus res;

  // memset (uncompressed_data, 0, max_uncompressed_length);
  memset (compressed_data, 0, squash_codec_get_max_compressed_size (codec, max_uncompressed_length));

  for ( uncompressed_length = 1 ;
        uncompressed_length <= max_uncompressed_length ;
        uncompressed_length += g_test_quick () ? g_test_rand_int_range (32, 128) : 1 ) {
    for ( ; uncompressed_data_filled < uncompressed_length ; uncompressed_data_filled++ ) {
      uncompressed_data[uncompressed_data_filled] = (uint8_t) g_test_rand_int_range (0x00, 0xff);
    }

    compressed_length = squash_codec_get_max_compressed_size (codec, uncompressed_length);
    g_assert (compressed_length > 0);

    res = squash_codec_compress (codec, &compressed_length, compressed_data, uncompressed_length, (uint8_t*) uncompressed_data, NULL);
    SQUASH_ASSERT_OK(res);
    g_assert (compressed_length > 0);
    g_assert (compressed_length <= squash_codec_get_max_compressed_size (codec, uncompressed_length));

    // Helpful when adding new codecs which don't document this…
    // g_message ("%zu -> %zu (%zu)", uncompressed_length, compressed_length, compressed_length - uncompressed_length);

    decompressed_length = uncompressed_length;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
    SQUASH_ASSERT_OK(res);
    g_assert (decompressed_length == uncompressed_length);

    int memcmpres = memcmp (uncompressed_data, decompressed_data, uncompressed_length);
    if (memcmpres != 0) {
      size_t pos;
      for (pos = 0 ; pos < decompressed_length ; pos++ ) {
        if (uncompressed_data[pos] != decompressed_data[pos]) {
          fprintf (stderr, "failed at %zu of %zu\n", pos, decompressed_length);
          break;
        }
      }
    }
    g_assert (memcmp (uncompressed_data, decompressed_data, uncompressed_length) == 0);

    /* How about decompressing some random data?  Note that this may
       actually succeed, so don't check the response—we just want to
       make sure it doesn't crash. */
    compressed_length = uncompressed_length;
    memcpy (compressed_data, uncompressed_data, uncompressed_length);
    squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
  }

  free (compressed_data);
  free (decompressed_data);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/random-data/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
