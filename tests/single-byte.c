#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  uint8_t uncompressed = (uint8_t) g_test_rand_int_range (0x00, 0xff);
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, 1);
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t decompressed;
  size_t decompressed_length = 1;
  SquashStatus res;

  res = squash_codec_compress_with_options (codec, compressed, &compressed_length, &uncompressed, 1, NULL);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, &decompressed, &decompressed_length, compressed, compressed_length, NULL);
  SQUASH_ASSERT_OK(res);
  g_assert (decompressed_length == 1);

  g_assert (uncompressed == decompressed);

  free (compressed);
}
