#include "test-codecs.h"

static void
flush_test (SquashCodec* codec) {
  SquashStream* compress = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
  SquashStream* decompress = squash_codec_create_stream (codec, SQUASH_STREAM_DECOMPRESS, NULL);
  uint8_t compressed[4096] = { 0, };
  uint8_t decompressed[LOREM_IPSUM_LENGTH] = { 0 };
  size_t uncompressed_bp = g_test_rand_int_range (1, LOREM_IPSUM_LENGTH - 1);
  size_t compressed_bp;
  SquashStatus status;

  compress->next_in = (uint8_t*) LOREM_IPSUM;
  compress->avail_in = uncompressed_bp;
  compress->next_out = compressed;
  compress->avail_out = sizeof (compressed);
  decompress->next_in = compressed;
  decompress->avail_in = 0;
  decompress->next_out = decompressed;
  decompress->avail_out = sizeof (decompressed);

  do {
    status = squash_stream_flush (compress);
  } while (status == SQUASH_PROCESSING);

  g_assert (status == SQUASH_OK);

  compressed_bp = compress->total_out;
  decompress->avail_in = compressed_bp;

  do {
    status = squash_stream_process (decompress);
  } while (status == SQUASH_PROCESSING);

  g_assert_cmpint (status, ==, SQUASH_OK);
  g_assert_cmpint (decompress->total_out, ==, uncompressed_bp);
  g_assert (memcmp (decompressed, LOREM_IPSUM, decompress->total_out) == 0);

  compress->avail_in = LOREM_IPSUM_LENGTH - compress->total_in;

  do {
    status = squash_stream_finish (compress);
  } while (status == SQUASH_PROCESSING);

  g_assert (status == SQUASH_OK);

  decompress->avail_in = compress->total_out - compressed_bp;

  do {
    status = squash_stream_process (decompress);
  } while (status == SQUASH_PROCESSING);


  if (status == SQUASH_END_OF_STREAM) {
    status = SQUASH_OK;
  } else if (status > 0) {
    do {
      status = squash_stream_finish (decompress);
    } while (status == SQUASH_PROCESSING);
  }

  g_assert_cmpint (status, ==, SQUASH_OK);
  g_assert_cmpint (decompress->total_out, ==, LOREM_IPSUM_LENGTH);
  g_assert (memcmp (decompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH) == 0);

  squash_object_unref (decompress);
  squash_object_unref (compress);
}

void
check_codec (SquashCodec* codec) {
  if ((squash_codec_get_info (codec) & SQUASH_CODEC_INFO_CAN_FLUSH) == SQUASH_CODEC_INFO_CAN_FLUSH)
    flush_test (codec);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/flush/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
