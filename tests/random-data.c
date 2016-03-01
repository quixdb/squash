#include "test-squash.h"

#define INPUT_BUF_SIZE ((size_t) (1024 * 1024 * 3))

static MunitResult
squash_test_random_compress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  size_t uncompressed_length;
  uint8_t* uncompressed_data = munit_newa (uint8_t, INPUT_BUF_SIZE);
  const size_t max_compressed_size = squash_codec_get_max_compressed_size (codec, INPUT_BUF_SIZE);
  size_t compressed_length;
  uint8_t* compressed_data = munit_newa (uint8_t, max_compressed_size);
  size_t decompressed_length;
  uint8_t* decompressed_data = munit_newa (uint8_t, INPUT_BUF_SIZE);

  munit_rand_memory(INPUT_BUF_SIZE, uncompressed_data);

  for (uncompressed_length = 1 ;
       uncompressed_length < INPUT_BUF_SIZE ;
       uncompressed_length += munit_rand_int_range (256, 1024) * (2 + (uncompressed_length / 512))) {
    const size_t req_max = squash_codec_get_max_compressed_size (codec, uncompressed_length);
    compressed_length = req_max;
    munit_assert_size (compressed_length, <=, max_compressed_size);
    munit_assert_size (compressed_length, >, 0);

    SquashStatus res = squash_codec_compress (codec, &compressed_length, compressed_data, uncompressed_length, uncompressed_data, NULL);
    /* Helpful when adding new codecs which don't document this… */
    munit_logf (MUNIT_LOG_DEBUG, "%" MUNIT_SIZE_MODIFIER "u -> %" MUNIT_SIZE_MODIFIER "u "
                "(%" MUNIT_SIZE_MODIFIER "u of %" MUNIT_SIZE_MODIFIER "u used, %" MUNIT_SIZE_MODIFIER "u extra)",
                uncompressed_length, compressed_length,
                compressed_length - uncompressed_length, req_max - uncompressed_length, req_max - compressed_length);
    SQUASH_ASSERT_OK(res);

    decompressed_length = uncompressed_length;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
    SQUASH_ASSERT_OK (res);
    munit_assert_size (decompressed_length, ==, uncompressed_length);

    munit_assert_memory_equal (decompressed_length, decompressed_data, uncompressed_data);
  }

  free (uncompressed_data);
  free (compressed_data);
  free (decompressed_data);

  return MUNIT_OK;
}

static MunitResult
squash_test_random_decompress(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  if ((squash_codec_get_info (codec) & SQUASH_CODEC_INFO_DECOMPRESS_UNSAFE) != 0)
    return MUNIT_OK;

  size_t compressed_length;
  uint8_t* compressed_data = munit_newa (uint8_t, INPUT_BUF_SIZE);
  size_t decompressed_length;
  uint8_t* decompressed_data = munit_newa (uint8_t, INPUT_BUF_SIZE);

  for (compressed_length = 1 ;
       compressed_length < INPUT_BUF_SIZE ;
       compressed_length += munit_rand_int_range (256, 1024) * (2 + (compressed_length / 512))) {
    decompressed_length = INPUT_BUF_SIZE;
    munit_rand_memory(compressed_length, compressed_data);
    squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
  }

  free (decompressed_data);
  free (compressed_data);

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
