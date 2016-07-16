/* Copyright (c) 2013-2015 The Squash Authors
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Evan Nemerson <evan@nemerson.com>
 */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <squash/squash.h>
#include <snappy-c.h>

SquashStatus squash_plugin_init_snappy_framed_codec (SquashCodec* codec,
                                                     SquashCodecImpl* impl);

SQUASH_PLUGIN_EXPORT
SquashStatus                     squash_plugin_init_codec       (SquashCodec* codec,
                                                                 SquashCodecImpl* impl);

static size_t
squash_snappy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return snappy_max_compressed_length (uncompressed_length);
}

static size_t
squash_snappy_get_uncompressed_size (SquashCodec* codec,
                                     size_t compressed_length,
                                     const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_length)]) {
  size_t uncompressed_size = 0;

  snappy_uncompressed_length ((const char*) compressed, compressed_length, &uncompressed_size);

  return uncompressed_size;
}

static SquashStatus
squash_snappy_status (snappy_status status) {
  SquashStatus res;

  switch ((int) status) {
    case SNAPPY_OK:
      res = SQUASH_OK;
      break;
    case SNAPPY_BUFFER_TOO_SMALL:
      res = squash_error (SQUASH_BUFFER_FULL);
      break;
    default:
      res = squash_error (SQUASH_FAILED);
      break;
  }

  return res;
}

static SquashStatus
squash_snappy_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_length,
                                 uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_length)],
                                 size_t compressed_length,
                                 const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_length)],
                                 SquashOptions* options) {
  snappy_status e;

  e = snappy_uncompress ((char*) compressed, compressed_length,
                         (char*) decompressed, decompressed_length);

  return squash_snappy_status (e);
}

static SquashStatus
squash_snappy_compress_buffer (SquashCodec* codec,
                               size_t* compressed_length,
                               uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_length)],
                               size_t uncompressed_length,
                               const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_length)],
                               SquashOptions* options) {
  snappy_status e;

  e = snappy_compress ((char*) uncompressed, uncompressed_length,
                       (char*) compressed, compressed_length);

  return squash_snappy_status (e);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("snappy", name) == 0) {
    impl->get_uncompressed_size = squash_snappy_get_uncompressed_size;
    impl->get_max_compressed_size = squash_snappy_get_max_compressed_size;
    impl->decompress_buffer = squash_snappy_decompress_buffer;
    impl->compress_buffer = squash_snappy_compress_buffer;
#if defined(SQUASH_SNAPPY_ENABLE_FRAMED)
  } else if (strcmp ("snappy-framed", name) == 0) {
    return squash_plugin_init_snappy_framed (codec, imp);
#endif
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
