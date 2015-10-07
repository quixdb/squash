/* Copyright (c) 2015 The Squash Authors
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

#include "pithy/pithy.h"

#define SQUASH_PITHY_DEFAULT_LEVEL 3

enum SquashPithyOptIndex {
  SQUASH_PITHY_OPT_LEVEL = 0,
};

static SquashOptionInfo squash_pithy_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 9 },
    .default_value.int_value = 3 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec,
                                                         SquashCodecImpl* impl);

static size_t
squash_pithy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return pithy_MaxCompressedLength (uncompressed_size);
}

static size_t
squash_pithy_get_uncompressed_size (SquashCodec* codec,
                                    size_t compressed_size,
                                    const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)]) {
  size_t uncompressed_size = 0;

  pithy_GetDecompressedLength ((const char*) compressed, compressed_size, &uncompressed_size);

  return uncompressed_size;
}

static SquashStatus
squash_pithy_compress_buffer (SquashCodec* codec,
                              size_t* compressed_size,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_PITHY_OPT_LEVEL);
  *compressed_size = pithy_Compress ((const char*) uncompressed, uncompressed_size, (char*) compressed, *compressed_size, level);
  return (*compressed_size != 0) ? SQUASH_OK : SQUASH_FAILED;
}

static SquashStatus
squash_pithy_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                SquashOptions* options) {
  size_t outlen = squash_pithy_get_uncompressed_size(codec, compressed_size, compressed);
  if (*decompressed_size < outlen)
    return SQUASH_BUFFER_FULL;

  if (pithy_Decompress ((const char*) compressed, compressed_size, (char*) decompressed, outlen)) {
    *decompressed_size = outlen;
    return SQUASH_OK;
  } else {
    return SQUASH_FAILED;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("pithy", name) == 0) {
    impl->options = squash_pithy_options;
    impl->get_uncompressed_size = squash_pithy_get_uncompressed_size;
    impl->get_max_compressed_size = squash_pithy_get_max_compressed_size;
    impl->decompress_buffer = squash_pithy_decompress_buffer;
    impl->compress_buffer = squash_pithy_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
