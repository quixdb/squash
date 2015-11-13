#include "test-codecs.h"

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
    g_assert_cmpint (*length, <=, remaining);
  } else {
    if (*length > remaining)
      *length = remaining;
    g_assert_cmpint (remaining, !=, 0);
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
    g_assert (*length <= remaining);
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

void
check_codec (SquashCodec* codec) {
  if (strcmp (squash_codec_get_name (codec), "density") == 0) {
#if defined(GLIB_VERSION_2_38)
    g_test_skip ("https://github.com/centaurean/density/issues/53");
#endif
    return;
  }

  const size_t max_compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  struct SpliceBuffers data = {
    SQUASH_STREAM_COMPRESS,
    (uint8_t*) LOREM_IPSUM,
    LOREM_IPSUM_LENGTH,
    0,
    g_malloc (max_compressed_length),
    squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH),
    0
  };

  g_assert (data.output != NULL);

  const size_t slen1 = (size_t) g_test_rand_int_range (1024, 2048);
  const size_t slen2 = (size_t) g_test_rand_int_range ( 512, 1024);

  SquashStatus res = squash_splice_custom (codec, SQUASH_STREAM_COMPRESS, write_cb, read_cb, &data, slen1, NULL);
  SQUASH_ASSERT_OK (res);

  {
    size_t decompressed_length = slen1;
    uint8_t* decompressed = g_malloc (slen1);
    res = squash_codec_decompress (codec, &decompressed_length, decompressed, data.output_pos, data.output, NULL);
    SQUASH_ASSERT_OK (res);

    g_assert_cmpint (decompressed_length, ==, slen1);
    g_assert (memcmp (decompressed, LOREM_IPSUM, slen1) == 0);

    g_free (decompressed);
  }

  data.stream_type = SQUASH_STREAM_DECOMPRESS;
  data.input = data.output;
  data.input_length = data.output_pos;
  data.input_pos = 0;
  data.output = g_malloc (slen2);
  data.output_length = slen2;
  data.output_pos = 0;

  res = squash_splice_custom (codec, SQUASH_STREAM_DECOMPRESS, write_cb, read_cb, &data, slen2, NULL);
  SQUASH_ASSERT_OK (res);
  g_assert_cmpint (data.output_pos, ==, slen2);

  g_assert (memcmp (LOREM_IPSUM, data.output, slen2) == 0);

  g_free (data.input);
  g_free (data.output);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/splice/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
