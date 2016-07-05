#include "test-squash.h"

static SquashStatus
buffer_to_buffer_compress_with_stream (SquashCodec* codec,
                                       size_t* compressed_length,
                                       uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_length)],
                                       size_t uncompressed_length,
                                       const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_length)]) {
  size_t step_size = munit_rand_int_range (64, 255);
  SquashStream* stream = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
  SquashStatus res;

	stream->next_out = compressed;
	stream->avail_out = (step_size < *compressed_length) ? step_size : *compressed_length;
  stream->next_in = uncompressed;

  while (stream->total_in < uncompressed_length) {
    stream->avail_in = MIN(uncompressed_length - stream->total_in, step_size);

    do {
      res = squash_stream_process (stream);

      if (stream->avail_out < step_size) {
        stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);
      }
    } while (res == SQUASH_PROCESSING);

    SQUASH_ASSERT_OK(res);
  }

  do {
    stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);

    res = squash_stream_finish (stream);
	} while (res == SQUASH_PROCESSING);

  if (res == SQUASH_OK) {
    *compressed_length = stream->total_out;
  }

  squash_object_unref (stream);

  return res;
}

static MunitResult
squash_test_stream_compress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = munit_malloc (compressed_length);
  uint8_t* uncompressed = munit_malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  res = buffer_to_buffer_compress_with_stream (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress (codec, &uncompressed_length, uncompressed, compressed_length, compressed, NULL);
  SQUASH_ASSERT_OK(res);
  munit_assert_size (uncompressed_length, ==, LOREM_IPSUM_LENGTH);

  munit_assert_memory_equal (uncompressed_length, uncompressed, LOREM_IPSUM);

  free (compressed);
  free (uncompressed);

  return MUNIT_OK;
}

static SquashStatus
buffer_to_buffer_decompress_with_stream (SquashCodec* codec,
                                         size_t* decompressed_length,
                                         uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_length)],
                                         size_t compressed_length,
                                         const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_length)]) {
  size_t step_size = munit_rand_int_range (64, 255);
  SquashStream* stream = squash_codec_create_stream (codec, SQUASH_STREAM_DECOMPRESS, NULL);
  SquashStatus res = SQUASH_OK;

	stream->next_out = decompressed;
	stream->avail_out = (step_size < *decompressed_length) ? step_size : *decompressed_length;
  stream->next_in = compressed;

  while (stream->total_in < compressed_length &&
         stream->total_out < *decompressed_length) {
    stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
    stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

    res = squash_stream_process (stream);
    if (res == SQUASH_END_OF_STREAM || res < 0) {
      break;
    }
  }

  if (res == SQUASH_END_OF_STREAM) {
    res = SQUASH_OK;
  } else if (res > 0) {
    do {
      stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
      stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);
  }

  if (res == SQUASH_OK) {
    *decompressed_length = stream->total_out;
  }

  squash_object_unref (stream);

  return res;
}

static MunitResult
squash_test_stream_decompress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  uint8_t* decompressed;
  size_t decompressed_length;
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  uint8_t* compressed = munit_malloc (compressed_length);
  SquashStatus res;

  res = squash_codec_compress (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM, NULL);
  SQUASH_ASSERT_OK(res);

  if ((squash_codec_get_info (codec) & SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) == SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) {
    decompressed_length = squash_codec_get_uncompressed_size (codec, compressed_length, compressed);
    munit_assert_size (decompressed_length, ==, LOREM_IPSUM_LENGTH);
  } else {
    decompressed_length = LOREM_IPSUM_LENGTH;
  }
  decompressed = munit_malloc (decompressed_length);

  res = buffer_to_buffer_decompress_with_stream (codec, &decompressed_length, decompressed, compressed_length, compressed);
  SQUASH_ASSERT_OK(res);

  munit_assert_size (decompressed_length, ==, LOREM_IPSUM_LENGTH);
  munit_assert_memory_equal(decompressed_length, decompressed, LOREM_IPSUM);

  free (compressed);
  free (decompressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_stream_single_byte(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

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
        munit_assert_size (stream->avail_out, !=, 0);
        res = squash_stream_process (stream);
      } while (res == SQUASH_PROCESSING);

      SQUASH_ASSERT_OK(res);
    }

    do {
      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);

    decompressed_length = sizeof(decompressed);
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, stream->total_out, compressed, NULL);
    SQUASH_ASSERT_OK(res);
    munit_assert_size (decompressed_length, ==, LOREM_IPSUM_LENGTH);
    munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

    squash_object_unref (stream);
  }

  { // Compress with 1 byte output
    stream = squash_codec_create_stream (codec, SQUASH_STREAM_COMPRESS, NULL);
    stream->next_out = compressed;
    stream->avail_in = LOREM_IPSUM_LENGTH;
    stream->next_in = (uint8_t*) LOREM_IPSUM;
    while (stream->total_in < LOREM_IPSUM_LENGTH) {
      do {
        munit_assert_size (stream->total_out, <, sizeof(compressed));
        stream->avail_out = 1;
        res = squash_stream_process (stream);
      } while (res == SQUASH_PROCESSING);

      SQUASH_ASSERT_OK(res);
    }

    do {
      munit_assert_size (stream->total_out, <, sizeof(compressed));
      stream->avail_out = 1;
      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);

    decompressed_length = LOREM_IPSUM_LENGTH;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, stream->total_out, compressed, NULL);
    SQUASH_ASSERT_OK(res);
    munit_assert_size (decompressed_length, ==, LOREM_IPSUM_LENGTH);
    munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

    squash_object_unref (stream);
  }

  // TODO: should probably test decompressing to a singe byte

  return MUNIT_OK;
}

MunitTest squash_stream_tests[] = {
  { (char*) "/compress", squash_test_stream_compress, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/decompress", squash_test_stream_decompress, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/single-byte", squash_test_stream_single_byte, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_stream = {
  (char*) "/stream",
  squash_stream_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
