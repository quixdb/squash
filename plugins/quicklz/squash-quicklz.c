/* Copyright (c) 2013-2016 The Squash Authors
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <squash/squash.h>

#include "quicklz.h"

typedef struct SquashQuickLZOptions_s {
  SquashOptions base_object;

  int level;
} SquashQuickLZOptions;

SQUASH_PLUGIN_EXPORT
SquashStatus                 squash_plugin_init_codec       (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_quicklz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 400;
}

/* We use this because qlz_size_decompressed and qlz_size_compressed
   can read outside the provided source buffer.  */
static bool squash_qlz_sizes(size_t source_length, const uint8_t source[HEDLEY_ARRAY_PARAM(source_length)],
                             size_t* decompressed_size, size_t* compressed_size) {
  uint32_t n = (((*source) & 2) == 2) ? 4 : 1;
  if (HEDLEY_UNLIKELY(source_length < ((2 * n) + 1)))
    return false;

  if(n == 4) {
    *compressed_size   = (source[1] | source[2] << 8 | source[3] << 16 | source[4] << 24);
    *decompressed_size = (source[5] | source[6] << 8 | source[7] << 16 | source[8] << 24);
  } else {
    *compressed_size = source[1];
    *decompressed_size = source[2];
  }

  return true;
}

static size_t
squash_quicklz_get_uncompressed_size (SquashCodec* codec,
                                      size_t compressed_size,
                                      const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)]) {
  size_t compressed_l, decompressed_l;

  if (!HEDLEY_LIKELY(squash_qlz_sizes(compressed_size, compressed, &decompressed_l, &compressed_l)))
    return (squash_error (SQUASH_BUFFER_EMPTY)), 0;

  return decompressed_l;
}

static SquashStatus
squash_quicklz_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_size,
                                  uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                                  size_t compressed_size,
                                  const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                                  SquashOptions* options) {
  size_t decompressed_l, compressed_l;
  qlz_state_decompress qlz_s;

  if (!HEDLEY_LIKELY(squash_qlz_sizes(compressed_size, compressed, &decompressed_l, &compressed_l)))
    return squash_error (SQUASH_BUFFER_EMPTY);

  if (HEDLEY_UNLIKELY(compressed_size < compressed_l))
    return squash_error (SQUASH_BUFFER_EMPTY);
  if (HEDLEY_UNLIKELY(*decompressed_size < decompressed_l))
    return squash_error (SQUASH_BUFFER_FULL);

  *decompressed_size = qlz_decompress ((const char*) compressed,
                                       (void*) decompressed,
                                       &qlz_s);

  return HEDLEY_LIKELY(decompressed_l == *decompressed_size) ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

static SquashStatus
squash_quicklz_compress_buffer (SquashCodec* codec,
                                size_t* compressed_size,
                                uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                                size_t uncompressed_size,
                                const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                                SquashOptions* options) {
  qlz_state_compress qlz_s;

  if (HEDLEY_UNLIKELY(*compressed_size < squash_quicklz_get_max_compressed_size (codec, uncompressed_size))) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  *compressed_size = qlz_compress ((const void*) uncompressed,
                                     (char*) compressed,
                                     uncompressed_size,
                                     &qlz_s);

  return HEDLEY_UNLIKELY(*compressed_size == 0) ? squash_error (SQUASH_FAILED) : SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("quicklz", name) == 0)) {
    impl->get_uncompressed_size = squash_quicklz_get_uncompressed_size;
    impl->get_max_compressed_size = squash_quicklz_get_max_compressed_size;
    impl->decompress_buffer = squash_quicklz_decompress_buffer;
    impl->compress_buffer = squash_quicklz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
