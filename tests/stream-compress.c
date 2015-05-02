#include "test-codecs.h"

static SquashStatus
buffer_to_buffer_compress_with_stream (SquashCodec* codec,
                                       size_t* compressed_length,
                                       uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                       size_t uncompressed_length,
                                       const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)]) {
  size_t step_size = g_test_rand_int_range (64, 255);
  SquashStream* stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_COMPRESS, NULL);
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

void
check_codec (SquashCodec* codec) {
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t* uncompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  res = buffer_to_buffer_compress_with_stream (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, &uncompressed_length, uncompressed, compressed_length, compressed, NULL);
  SQUASH_ASSERT_OK(res);
  g_assert (uncompressed_length == LOREM_IPSUM_LENGTH);

  g_assert (memcmp (LOREM_IPSUM, uncompressed, uncompressed_length) == 0);

  free (compressed);
  free (uncompressed);
}
