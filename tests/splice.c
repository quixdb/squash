#include "test-squash.h"

struct SpliceBuffers {
  SquashStreamType stream_type;

  uint8_t* input;
  size_t input_length;
  size_t input_pos;

  uint8_t* output;
  size_t output_length;
  size_t output_pos;
};

static SquashStatus
write_cb (size_t* length, const uint8_t* buffer, void* user_data) {
  struct SpliceBuffers* data = (struct SpliceBuffers*) user_data;

  const size_t remaining = data->output_length - data->output_pos;

  assert (*length < 1024 * 1024);

  if (data->stream_type == SQUASH_STREAM_DECOMPRESS) {
    munit_assert_size (*length, <=, remaining);
  } else {
    if (*length > remaining)
      *length = remaining;
    munit_assert_size (remaining, !=, 0);
  }

  memcpy (data->output + data->output_pos, buffer, *length);
  data->output_pos += *length;

  return SQUASH_OK;
}

static SquashStatus
read_cb (size_t* length, uint8_t* buffer, void* user_data) {
  struct SpliceBuffers* data = (struct SpliceBuffers*) user_data;

  const size_t remaining = data->input_length - data->input_pos;

  if (data->stream_type == SQUASH_STREAM_COMPRESS)
    munit_assert_size (*length, <=, remaining);
  else if (*length > remaining)
    *length = remaining;

  if (*length != 0) {
    memcpy (buffer, data->input + data->input_pos, *length);
    data->input_pos += *length;
    return SQUASH_OK;
  } else {
    return SQUASH_END_OF_STREAM;
  }
}

static MunitResult
squash_test_custom(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_not_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  if (strcmp (squash_codec_get_name (codec), "density") == 0)
    return MUNIT_SKIP;

  const size_t max_compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  struct SpliceBuffers data = {
    SQUASH_STREAM_COMPRESS,
    (uint8_t*) LOREM_IPSUM,
    LOREM_IPSUM_LENGTH,
    0,
    munit_malloc (max_compressed_length),
    squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH),
    0
  };

  munit_assert_not_null (data.output);

  const size_t slen1 = (size_t) munit_rand_int_range (1024, 2048);
  const size_t slen2 = (size_t) munit_rand_int_range ( 512, 1024);

  SquashStatus res = squash_splice_custom (codec, SQUASH_STREAM_COMPRESS, write_cb, read_cb, &data, slen1, NULL);
  SQUASH_ASSERT_OK (res);

  {
    size_t decompressed_length = slen1;
    uint8_t* decompressed = munit_malloc (slen1);
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, data.output_pos, data.output, NULL);
    SQUASH_ASSERT_OK (res);

    munit_assert_size (decompressed_length, ==, slen1);
    munit_assert_memory_equal (slen1, decompressed, LOREM_IPSUM);

    free (decompressed);
  }

  data.stream_type = SQUASH_STREAM_DECOMPRESS;
  data.input = data.output;
  data.input_length = data.output_pos;
  data.input_pos = 0;
  data.output = munit_malloc (slen2);
  data.output_length = slen2;
  data.output_pos = 0;

  res = squash_splice_custom (codec, SQUASH_STREAM_DECOMPRESS, write_cb, read_cb, &data, slen2, NULL);
  SQUASH_ASSERT_OK (res);
  munit_assert_size (data.output_pos, ==, slen2);

  munit_assert_memory_equal(slen2, data.output, LOREM_IPSUM);

  free (data.input);
  free (data.output);

  return MUNIT_OK;
}

MunitTest squash_splice_tests[] = {
  { (char*) "/custom", squash_test_custom, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_splice = {
  (char*) "/splice",
  squash_splice_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
