#include "test-squash.h"

#define MAX_UNCOMPRESSED_LENGTH 4096

static MunitResult
squash_test_random_compress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  size_t uncompressed_length;
  uint8_t uncompressed_data[MAX_UNCOMPRESSED_LENGTH];
  const size_t max_compressed_size = squash_codec_get_max_compressed_size (codec, MAX_UNCOMPRESSED_LENGTH);
  size_t compressed_length;
  uint8_t* compressed_data = munit_newa (uint8_t, max_compressed_size);
  size_t decompressed_length;
  uint8_t decompressed_data[MAX_UNCOMPRESSED_LENGTH];

  munit_rand_memory(sizeof(uncompressed_data), uncompressed_data);

  for (uncompressed_length = 1 ;
       uncompressed_length < MAX_UNCOMPRESSED_LENGTH ;
       uncompressed_length += munit_rand_int_range (32, 128)) {
    compressed_length = squash_codec_get_max_compressed_size (codec, uncompressed_length);
    munit_assert_cmp_size (compressed_length, <=, max_compressed_size);
    munit_assert_cmp_size (compressed_length, >, 0);

    SquashStatus res = squash_codec_compress (codec, &compressed_length, compressed_data, uncompressed_length, uncompressed_data, NULL);
    /* Helpful when adding new codecs which don't document this… */
    munit_logf (MUNIT_LOG_DEBUG, "%" MUNIT_SIZE_MODIFIER "u -> %" MUNIT_SIZE_MODIFIER "u (%" MUNIT_SIZE_MODIFIER "u)",
                uncompressed_length, compressed_length, compressed_length - uncompressed_length);
    SQUASH_ASSERT_OK(res);

    decompressed_length = uncompressed_length;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
    SQUASH_ASSERT_OK (res);
    munit_assert_cmp_size (decompressed_length, ==, uncompressed_length);

    munit_assert_memory_equal (decompressed_length, decompressed_data, uncompressed_data);
  }

  return MUNIT_OK;
}

static MunitResult
squash_test_random_decompress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  size_t compressed_length;
  uint8_t compressed_data[MAX_UNCOMPRESSED_LENGTH];
  size_t decompressed_length;
  uint8_t decompressed_data[MAX_UNCOMPRESSED_LENGTH];

  for (compressed_length = 1 ;
       compressed_length < MAX_UNCOMPRESSED_LENGTH ;
       compressed_length += munit_rand_int_range (32, 128)) {
    decompressed_length = MAX_UNCOMPRESSED_LENGTH;
    munit_rand_memory(compressed_length, compressed_data);
    squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
  }

  return MUNIT_OK;
}

static MunitTest squash_random_tests[] = {
  { (char*) "/compress", squash_test_random_compress, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/decompress", squash_test_random_decompress, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_random = {
  (char*) "/random",
  squash_random_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
