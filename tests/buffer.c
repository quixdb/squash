#include "test-squash.h"

#include <stdio.h>

static MunitResult
squash_test_basic(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  if (strcmp ("lz4-raw", squash_codec_get_name (codec)) == 0)
    return MUNIT_SKIP;

  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t* uncompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  munit_assert_size(compressed_length, >=, LOREM_IPSUM_LENGTH);
  munit_assert_not_null(compressed);
  munit_assert_not_null(uncompressed);

  res = squash_codec_compress (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM, NULL);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress (codec, &uncompressed_length, uncompressed, compressed_length, compressed, NULL);
  munit_assert_int(LOREM_IPSUM_LENGTH, ==, uncompressed_length);
  SQUASH_ASSERT_OK(res);

  munit_assert_memory_equal(LOREM_IPSUM_LENGTH, uncompressed, LOREM_IPSUM);

  uncompressed_length = LOREM_IPSUM_LENGTH - 1;
  res = squash_codec_decompress (codec, &uncompressed_length, uncompressed, compressed_length, compressed, NULL);
  munit_assert_int (res, ==, SQUASH_BUFFER_FULL);

  free (compressed);
  free (uncompressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_single_byte(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  uint8_t uncompressed = (uint8_t) munit_rand_int_range (0x00, 0xff);
  uint8_t decompressed;
  uint8_t* compressed = NULL;
  size_t compressed_length;
  size_t decompressed_length = 1;

  compressed_length = squash_codec_get_max_compressed_size (codec, 1);
  munit_assert_size (compressed_length, >=, 1);
  compressed = munit_malloc (compressed_length);

  SquashStatus res = squash_codec_compress (codec, &compressed_length, compressed, 1, &uncompressed, NULL);
  SQUASH_ASSERT_OK(res);
  res = squash_codec_decompress (codec, &decompressed_length, &decompressed, compressed_length, compressed, NULL);
  SQUASH_ASSERT_OK(res);

  munit_assert_memory_equal(1, &uncompressed, &decompressed);

  free (compressed);

  return MUNIT_OK;
}

#if defined(SQUASH_TEST_DATA_DIR)

static MunitResult
squash_test_endianness(MUNIT_UNUSED const MunitParameter params[], void* user_data, bool le) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  uint8_t compressed[8192] = { 0, };
  size_t compressed_length = sizeof(compressed);
  uint8_t decompressed[LOREM_IPSUM_LENGTH] = { 0, };
  size_t decompressed_length = sizeof(decompressed);

  {
    char filename[8192];
#if !defined(_WIN32)
    const size_t filename_l = snprintf(filename, sizeof(filename), "%s/lipsum.%s.%s", SQUASH_TEST_DATA_DIR, le ? "le" : "be", squash_codec_get_name(codec));
#else
    const size_t filename_l = _snprintf(filename, sizeof(filename), "%s\\lipsum.%s.%s", SQUASH_TEST_DATA_DIR, le ? "le" : "be", squash_codec_get_name(codec));
#endif

    if (filename_l >= (sizeof(filename) - 1))
      return MUNIT_SKIP;

    FILE* fp = fopen(filename, "rb");
    if (HEDLEY_UNLIKELY(fp == NULL))
      return MUNIT_ERROR;

    compressed_length = fread(compressed, 1, compressed_length, fp);
    if (!feof(fp)) {
      fclose(fp);
      return MUNIT_ERROR;
    }
    fclose(fp);
  }

  SquashStatus res = squash_codec_decompress (codec, &decompressed_length, decompressed, compressed_length, compressed, NULL);
  SQUASH_ASSERT_OK(res);
  munit_assert_int(LOREM_IPSUM_LENGTH, ==, decompressed_length);

  munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

  return MUNIT_OK;
}

static MunitResult
squash_test_endianness_le(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  return squash_test_endianness(params, user_data, true);
}

/* static MunitResult */
/* squash_test_endianness_be(MUNIT_UNUSED const MunitParameter params[], void* user_data) { */
/*   return squash_test_endianness(params, user_data, false); */
/* } */

#endif

MunitTest squash_buffer_tests[] = {
  { (char*) "/basic", squash_test_basic, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/single-byte", squash_test_single_byte, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
#if defined(SQUASH_TEST_DATA_DIR)
  { (char*) "/endianness", squash_test_endianness_le, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  /* { (char*) "/endianness/be", squash_test_endianness_be, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER }, */
#endif
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_buffer = {
  (char*) "/buffer",
  squash_buffer_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
