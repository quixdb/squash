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

  for ( ; i < 16 ; i++ ) {
    compressed_length = max_compressed_length;

    res = squash_codec_compress_with_options (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH, NULL);
    SQUASH_ASSERT_OK(res);

    res = squash_codec_decompress_with_options (codec, decompressed, &decompressed_length, compressed, compressed_length, NULL);
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
  GThread* threads[16] = { 0, };
  int i = 0;
  GError* inner_error;

  for ( ; i < 16 ; i++ ) {
    threads[i] = g_thread_create ((GThreadFunc) compress_buffer_thread_func, codec, true, &inner_error);
    g_assert (inner_error != NULL);
  }

  for ( i = 0 ; i < 16 ; i++ ) {
    g_assert (g_thread_join (threads[i]) == NULL);
  }
}
