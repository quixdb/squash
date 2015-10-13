#include "test-codecs.h"

static SquashStatus
buffer_to_buffer_decompress_with_stream (SquashCodec* codec,
                                         size_t* decompressed_length,
                                         uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                         size_t compressed_length,
                                         const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  size_t step_size = g_test_rand_int_range (64, 255);
  SquashStream* stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_DECOMPRESS, NULL);
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

void
check_codec (SquashCodec* codec) {
  uint8_t* decompressed;
  size_t decompressed_length;
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  SquashStatus res;

  res = squash_codec_compress_with_options (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM, NULL);
  SQUASH_ASSERT_OK(res);

  if ((squash_codec_get_info (codec) & SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) == SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) {
    decompressed_length = squash_codec_get_uncompressed_size (codec, compressed_length, compressed);
    g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  } else {
    decompressed_length = LOREM_IPSUM_LENGTH;
  }
  decompressed = (uint8_t*) malloc (decompressed_length);

  res = buffer_to_buffer_decompress_with_stream (codec, &decompressed_length, decompressed, compressed_length, compressed);
  SQUASH_ASSERT_OK(res);

  g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  g_assert (memcmp (LOREM_IPSUM, decompressed, decompressed_length) == 0);

  free (compressed);
  free (decompressed);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/stream-decompress/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
