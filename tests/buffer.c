#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t* uncompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  res = squash_codec_compress_with_options (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH, NULL);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, uncompressed, &uncompressed_length, compressed, compressed_length, NULL);
  SQUASH_ASSERT_OK(res);
  g_assert (uncompressed_length == LOREM_IPSUM_LENGTH);

  g_assert (memcmp (LOREM_IPSUM, uncompressed, LOREM_IPSUM_LENGTH) == 0);

  free (compressed);
  free (uncompressed);
}
