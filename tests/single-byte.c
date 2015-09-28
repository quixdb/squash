#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  uint8_t uncompressed = (uint8_t) g_test_rand_int_range (0x00, 0xff);
  uint8_t compressed[8192];
  size_t compressed_length = sizeof(compressed);
  uint8_t decompressed;
  size_t decompressed_length = 1;
  SquashStatus res;

  g_assert_cmpint (squash_codec_get_max_compressed_size (codec, 1), <=, sizeof(compressed));

  if (strcmp (squash_codec_get_name (codec), "lzf") == 0)
    return;

  res = squash_codec_compress_with_options (codec, &compressed_length, compressed, 1, &uncompressed, NULL);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, &decompressed_length, &decompressed, compressed_length, compressed, NULL);
  SQUASH_ASSERT_OK(res);
  g_assert (decompressed_length == 1);

  g_assert (uncompressed == decompressed);
}
