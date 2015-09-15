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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <squash/squash.h>

#include "fastlz/fastlz.h"

enum SquashFastLZOptIndex {
  SQUASH_FASTLZ_OPT_LEVEL = 0
};

static SquashOptionInfo squash_fastlz_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 2 },
    .default_value.int_value = 1 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus                squash_plugin_init_codec      (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_fastlz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  size_t max_compressed_size = uncompressed_length +
    (uncompressed_length / 20) + ((uncompressed_length % 20 == 0) ? uncompressed_length / 20 : 0);

  return (max_compressed_size < 66) ? 66 : max_compressed_size;
}

static SquashStatus
squash_fastlz_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_length,
                                 uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                 size_t compressed_length,
                                 const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                 SquashOptions* options) {
  int fastlz_e = fastlz_decompress ((const void*) compressed,
                                    (int) compressed_length,
                                    (void*) decompressed,
                                    (int) *decompressed_length);

  if (fastlz_e < 0) {
    return squash_error (SQUASH_FAILED);
  } else if (fastlz_e == 0) {
    return SQUASH_BUFFER_FULL;
  } else {
    *decompressed_length = (size_t) fastlz_e;
    return SQUASH_OK;
  }
}

static SquashStatus
squash_fastlz_compress_buffer (SquashCodec* codec,
                               size_t* compressed_length,
                               uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                               size_t uncompressed_length,
                               const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                               SquashOptions* options) {
  *compressed_length = fastlz_compress_level (squash_codec_get_option_int_index (codec, options, SQUASH_FASTLZ_OPT_LEVEL),
                                              (const void*) uncompressed,
                                              (int) uncompressed_length,
                                              (void*) compressed);

  return (*compressed_length == 0) ? squash_error (SQUASH_FAILED) : SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("fastlz", name) == 0) {
    impl->options = squash_fastlz_options;
    impl->get_max_compressed_size = squash_fastlz_get_max_compressed_size;
    impl->decompress_buffer = squash_fastlz_decompress_buffer;
    impl->compress_buffer_unsafe = squash_fastlz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
