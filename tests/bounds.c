#include "test-codecs.h"

#include <errno.h>
#include <string.h>

void
check_codec (SquashCodec* codec) {
  uint8_t* uncompressed = NULL;
  uint8_t* compressed = NULL;
  uint8_t* decompressed = NULL;

  SquashStatus res;
  size_t compressed_length;
  size_t decompressed_length;
  size_t real_compressed_length;

  if (strcmp (squash_codec_get_name (codec), "density") == 0) {
#if defined(GLIB_VERSION_2_38)
    g_test_skip ("https://github.com/centaurean/density/issues/53");
#endif
    return;
  }

  uncompressed = malloc (LOREM_IPSUM_LENGTH);
  memcpy (uncompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH);

  compressed = malloc (8192);
  compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  g_assert (compressed_length < 8192);

  /* Determine the true size of the compressed data. */
  res = squash_codec_compress (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, uncompressed, NULL);
  g_assert (res == SQUASH_OK);
  real_compressed_length = compressed_length;

  /* Decompress to a buffer which is exactly the right size */
  decompressed = malloc (LOREM_IPSUM_LENGTH);
  decompressed_length = LOREM_IPSUM_LENGTH;
  res = squash_codec_decompress (codec, &decompressed_length, decompressed, compressed_length, compressed, NULL);
  g_assert (res == SQUASH_OK);

  /* Decompress to a buffer which is barely too small */
  decompressed_length = LOREM_IPSUM_LENGTH - 1;
  res = squash_codec_decompress (codec, &decompressed_length, decompressed + 1, compressed_length, compressed, NULL);
  g_assert (res != SQUASH_OK);

  /* Decompress to a buffer which is significantly too small */
  decompressed_length = (size_t) g_test_rand_int_range (1, LOREM_IPSUM_LENGTH - 1);
  res = squash_codec_decompress (codec, &decompressed_length, decompressed + (LOREM_IPSUM_LENGTH - decompressed_length), compressed_length, compressed, NULL);
  g_assert (res != SQUASH_OK);

  /* Compress to a buffer which is exactly the right size */
  compressed_length = real_compressed_length;
  res = squash_codec_compress (codec, &compressed_length, compressed + (8192 - real_compressed_length), LOREM_IPSUM_LENGTH, uncompressed, NULL);
  /* It's okay if some codecs require a few extra bytes to *compress*,
     as long as they don't write outside the buffer they were provided,
     so don't check the return value here. */

  /* Compress to a buffer which is barely too small */
  compressed_length = real_compressed_length - 1;
  res = squash_codec_compress (codec, &compressed_length, compressed + (8192 - (real_compressed_length - 1)), LOREM_IPSUM_LENGTH, uncompressed, NULL);
  g_assert (res != SQUASH_OK);

  free (uncompressed);
  free (compressed);
  free (decompressed);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/bounds/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
