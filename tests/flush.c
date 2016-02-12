#include "test-squash.h"

static MunitResult
squash_test_flush(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  if ((squash_codec_get_info (codec) & SQUASH_CODEC_INFO_CAN_FLUSH) != SQUASH_CODEC_INFO_CAN_FLUSH)
    return MUNIT_OK;

  SquashStream* compress = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
  SquashStream* decompress = squash_codec_create_stream (codec, SQUASH_STREAM_DECOMPRESS, NULL);
  uint8_t compressed[4096] = { 0, };
  uint8_t decompressed[LOREM_IPSUM_LENGTH] = { 0 };
  size_t uncompressed_bp = munit_rand_int_range (1, LOREM_IPSUM_LENGTH - 1);
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

  SQUASH_ASSERT_OK(status);

  compressed_bp = compress->total_out;
  decompress->avail_in = compressed_bp;

  do {
    status = squash_stream_process (decompress);
  } while (status == SQUASH_PROCESSING);

  SQUASH_ASSERT_OK(status);
  munit_assert_size (decompress->total_out, ==, uncompressed_bp);
  munit_assert_memory_equal(decompress->total_out, decompressed, LOREM_IPSUM);

  compress->avail_in = LOREM_IPSUM_LENGTH - compress->total_in;

  do {
    status = squash_stream_finish (compress);
  } while (status == SQUASH_PROCESSING);

  SQUASH_ASSERT_OK(status);

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

  SQUASH_ASSERT_OK(status);
  munit_assert_size (decompress->total_out, ==, LOREM_IPSUM_LENGTH);
  munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

  squash_object_unref (decompress);
  squash_object_unref (compress);

  return MUNIT_OK;
}

MunitTest squash_flush_tests[] = {
  { (char*) "/flush", squash_test_flush, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_flush = {
  (char*) "",
  squash_flush_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
