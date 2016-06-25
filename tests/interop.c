#include "test-squash.h"

struct SquashInteropData {
  void (* func) (struct SquashInteropData* data, SquashCodec* codec);
  SquashCodec* codec;
  size_t compressed_length;
  uint8_t* compressed;
};

static void
squash_test_basic (struct SquashInteropData* data, SquashCodec* codec) {
  SquashStatus res;

  if (data->compressed == NULL) {
    data->compressed_length = squash_codec_get_max_compressed_size (data->codec, LOREM_IPSUM_LENGTH);
    data->compressed = munit_malloc(data->compressed_length);
    res =
      squash_codec_compress (data->codec,
                             &(data->compressed_length), data->compressed,
                             LOREM_IPSUM_LENGTH, LOREM_IPSUM,
                             NULL);
    SQUASH_ASSERT_OK(res);
  }

  size_t decompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* decompressed = munit_malloc(decompressed_length);

  res = squash_codec_decompress (codec,
                                 &decompressed_length, decompressed,
                                 data->compressed_length, data->compressed,
                                 NULL);
  SQUASH_ASSERT_OK(res);

  munit_assert_size(decompressed_length, ==, LOREM_IPSUM_LENGTH);
  munit_assert_memory_equal(decompressed_length, decompressed, LOREM_IPSUM);

  free (decompressed);
}

static void
squash_test_interop_codec_cb (SquashCodec* codec, void* user_data) {
  struct SquashInteropData* data = (struct SquashInteropData*) user_data;

  if (data->codec != codec &&
      strcmp (squash_codec_get_name (data->codec),
              squash_codec_get_name (codec)) == 0) {
    squash_test_basic (data, codec);
  }
}

static void
squash_test_interop_plugin_cb (SquashPlugin* plugin, void* user_data) {
  squash_plugin_foreach_codec (plugin, squash_test_interop_codec_cb, user_data);
}

static MunitResult
squash_test_basic_wrapper(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);

  struct SquashInteropData data = {
    squash_test_basic,
    (SquashCodec*) user_data,
    squash_codec_get_max_compressed_size ((SquashCodec*) user_data, LOREM_IPSUM_LENGTH),
    NULL
  };

  squash_foreach_plugin (squash_test_interop_plugin_cb, &data);

  free (data.compressed);

  return MUNIT_OK;
}

MunitTest squash_interop_tests[] = {
  { (char*) "/basic", squash_test_basic_wrapper, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_interop = {
  (char*) "/interop",
  squash_interop_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
