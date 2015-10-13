#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  uint8_t compressed[8192];
  uint8_t decompressed[8192];
  size_t decompressed_length;
  SquashStatus res;
  SquashStream* stream;

  { // Compress with 1 byte input
    stream = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
    stream->next_out = compressed;
    stream->avail_out = sizeof(compressed);
    stream->next_in = (uint8_t*) LOREM_IPSUM;
    while (stream->total_in < LOREM_IPSUM_LENGTH) {
      stream->avail_in = 1;

      do {
        g_assert (stream->avail_out != 0);
        res = squash_stream_process (stream);
      } while (res == SQUASH_PROCESSING);

      SQUASH_ASSERT_OK(res);
    }

    do {
      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);

    decompressed_length = sizeof(decompressed);
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, stream->total_out, compressed, NULL);
    g_assert_cmpint (res, ==, SQUASH_OK);
    g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
    g_assert (memcpy (decompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH));

    squash_object_unref (stream);
  }

  { // Compress with 1 byte output
    stream = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
    stream->next_out = compressed;
    stream->avail_in = LOREM_IPSUM_LENGTH;
    stream->next_in = (uint8_t*) LOREM_IPSUM;
    while (stream->total_in < LOREM_IPSUM_LENGTH) {
      do {
        g_assert (stream->total_out < sizeof(compressed));
        stream->avail_out = 1;
        res = squash_stream_process (stream);
      } while (res == SQUASH_PROCESSING);

      SQUASH_ASSERT_OK(res);
    }

    do {
      g_assert (stream->total_out < sizeof(compressed));
      stream->avail_out = 1;
      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);

    decompressed_length = LOREM_IPSUM_LENGTH;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, stream->total_out, compressed, NULL);
    g_assert_cmpint (res, ==, SQUASH_OK);
    g_assert_cmpint (decompressed_length, ==, LOREM_IPSUM_LENGTH);
    g_assert (memcpy (decompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH));

    squash_object_unref (stream);
  }

  // TODO: should probably test decompressing to a singe byte
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/stream-single-byte/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
