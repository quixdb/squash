#include "test-codecs.h"

void
check_codec (SquashCodec* codec) {
  SquashStatus status;
  size_t buffer_size = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t compressed_length = buffer_size;
  uint8_t* buffer = (uint8_t*) malloc (buffer_size);

  memcpy (buffer, LOREM_IPSUM, LOREM_IPSUM_LENGTH);

  SquashCodecFeatures features = squash_codec_get_features (codec);

  status = squash_codec_compress (codec, buffer, &compressed_length, buffer, LOREM_IPSUM_LENGTH, NULL);

  if (features & SQUASH_CODEC_FEATURE_COMPRESS_IN_PLACE) {
    SQUASH_ASSERT_OK(status);

    size_t decompressed_length = buffer_size;
    status = squash_codec_decompress (codec, buffer, &decompressed_length, buffer, compressed_length, NULL);

    if (features & SQUASH_CODEC_FEATURE_DECOMPRESS_IN_PLACE) {
      SQUASH_ASSERT_OK(status);
      SQUASH_ASSERT_VALUE(decompressed_length, LOREM_IPSUM_LENGTH);
      SQUASH_ASSERT_VALUE(memcmp (buffer, LOREM_IPSUM, LOREM_IPSUM_LENGTH), 0);
    } else {
      SQUASH_ASSERT_STATUS(status, SQUASH_INVALID_BUFFER);
    }
  } else {
    SQUASH_ASSERT_STATUS(status, SQUASH_INVALID_BUFFER);
  }

  free (buffer);
}
