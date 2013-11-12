#include "test-codecs.h"

static SquashStatus
buffer_to_buffer_decompress_with_stream (SquashCodec* codec,
                                         uint8_t* decompressed, size_t* decompressed_length,
                                         uint8_t* compressed, size_t compressed_length) {
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

  res = squash_codec_compress_with_options (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH, NULL);
  SQUASH_ASSERT_OK(res);

  if (squash_codec_get_knows_uncompressed_size (codec)) {
    decompressed_length = squash_codec_get_uncompressed_size (codec, compressed, compressed_length);
    g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  } else {
    decompressed_length = LOREM_IPSUM_LENGTH;
  }
  decompressed = (uint8_t*) malloc (decompressed_length);

  res = buffer_to_buffer_decompress_with_stream (codec, decompressed, &decompressed_length, compressed, compressed_length);
  SQUASH_ASSERT_OK(res);

  g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  g_assert (memcmp (LOREM_IPSUM, decompressed, decompressed_length) == 0);

  free (compressed);
  free (decompressed);
}
