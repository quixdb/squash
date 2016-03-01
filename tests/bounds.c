#include "test-squash.h"

struct BoundsInfo {
  SquashCodec* codec;
  uint8_t* compressed;
  size_t compressed_length;
};

static void*
squash_test_bounds_setup(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = munit_new (struct BoundsInfo);
  info->codec = squash_get_codec (munit_parameters_get(params, "codec"));
  munit_assert_not_null (info->codec);
  info->compressed_length = squash_codec_get_max_compressed_size (info->codec, LOREM_IPSUM_LENGTH);
  munit_assert_size (info->compressed_length, >=, LOREM_IPSUM_LENGTH);
  info->compressed = munit_malloc(info->compressed_length);

  SquashStatus res =
    squash_codec_compress (info->codec,
                           &(info->compressed_length), info->compressed,
                           LOREM_IPSUM_LENGTH, LOREM_IPSUM, NULL);
  SQUASH_ASSERT_OK (res);

  return info;
}

static void
squash_test_bounds_tear_down(void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);
  free (info->compressed);
  free (info);
}

static MunitResult
squash_test_bounds_decode_exact(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  size_t decompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* decompressed = munit_malloc(decompressed_length);
  SquashStatus res =
    squash_codec_decompress (info->codec,
                             &decompressed_length, decompressed,
                             info->compressed_length, info->compressed, NULL);
  SQUASH_ASSERT_OK(res);
  munit_assert_size(LOREM_IPSUM_LENGTH, ==, decompressed_length);

  munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

  free (decompressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_bounds_decode_small(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  /* *Almost* enough */
  size_t decompressed_length = (size_t) LOREM_IPSUM_LENGTH - 1;
  uint8_t* decompressed = munit_malloc(decompressed_length);
  SquashStatus res =
    squash_codec_decompress (info->codec,
                             &decompressed_length, decompressed,
                             info->compressed_length, info->compressed, NULL);
  munit_assert_int(res, <, 0);
  free (decompressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_bounds_decode_tiny(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  /* Between 1 and length - 1 bytes (usually way too small) */
  size_t decompressed_length = (size_t) munit_rand_int_range(1, LOREM_IPSUM_LENGTH - 1);
  uint8_t* decompressed = munit_malloc(decompressed_length);
  SquashStatus res =
    squash_codec_decompress (info->codec,
                             &decompressed_length, decompressed,
                             info->compressed_length, info->compressed, NULL);
  munit_assert_int(res, <, 0);
  free (decompressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_bounds_encode_exact(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  size_t compressed_length = info->compressed_length;
  uint8_t* compressed = munit_malloc(compressed_length);
  squash_codec_compress (info->codec,
                         &compressed_length, compressed,
                         LOREM_IPSUM_LENGTH, LOREM_IPSUM, NULL);
  /* It's okay if some codecs require a few extra bytes to *compress*,
     as long as they don't write outside the buffer they were provided,
     so don't check the return value here. */

  free(compressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_bounds_encode_small(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  size_t compressed_length = info->compressed_length - 1;
  uint8_t* compressed = munit_malloc(compressed_length);
  SquashStatus res =
    squash_codec_compress (info->codec,
                           &compressed_length, compressed,
                           LOREM_IPSUM_LENGTH, LOREM_IPSUM, NULL);
  munit_assert_int(res, <, 0);

  free(compressed);

  return MUNIT_OK;
}

static MunitResult
squash_test_bounds_encode_tiny(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  struct BoundsInfo* info = (struct BoundsInfo*) user_data;
  munit_assert_not_null (info);

  size_t compressed_length = munit_rand_int_range (1, info->compressed_length - 1);
  uint8_t* compressed = munit_malloc(compressed_length);
  SquashStatus res =
    squash_codec_compress (info->codec,
                           &compressed_length, compressed,
                           LOREM_IPSUM_LENGTH, LOREM_IPSUM, NULL);
  munit_assert_int(res, <, 0);

  free(compressed);

  return MUNIT_OK;
}

MunitTest squash_bounds_tests[] = {
  { (char*) "/decode/exact", squash_test_bounds_decode_exact, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/decode/small", squash_test_bounds_decode_small, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/decode/tiny", squash_test_bounds_decode_tiny, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/encode/exact", squash_test_bounds_encode_exact, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/encode/small", squash_test_bounds_encode_small, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/encode/tiny", squash_test_bounds_encode_tiny, squash_test_bounds_setup, squash_test_bounds_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_bounds = {
  (char*) "/bounds",
  squash_bounds_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
