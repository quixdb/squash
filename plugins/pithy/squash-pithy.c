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
                                                         SquashCodecFuncs* funcs);

static size_t
squash_pithy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return pithy_MaxCompressedLength (uncompressed_length);
}

static size_t
squash_pithy_get_uncompressed_size (SquashCodec* codec,
                                    size_t compressed_length,
                                    const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  size_t uncompressed_size = 0;

  pithy_GetDecompressedLength ((const char*) compressed, compressed_length, &uncompressed_size);

  return uncompressed_size;
}

static SquashStatus
squash_pithy_compress_buffer (SquashCodec* codec,
                              size_t* compressed_length,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                              size_t uncompressed_length,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                              SquashOptions* options) {
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_PITHY_OPT_LEVEL);
  *compressed_length = pithy_Compress ((const char*) uncompressed, uncompressed_length, (char*) compressed, *compressed_length, level);
  return (*compressed_length != 0) ? SQUASH_OK : SQUASH_FAILED;
}

static SquashStatus
squash_pithy_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_length,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                size_t compressed_length,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                SquashOptions* options) {
  size_t outlen = squash_pithy_get_uncompressed_size(codec, compressed_length, compressed);
  if (*decompressed_length < outlen)
    return SQUASH_BUFFER_FULL;

  if (pithy_Decompress ((const char*) compressed, compressed_length, (char*) decompressed, outlen)) {
    *decompressed_length = outlen;
    return SQUASH_OK;
  } else {
    return SQUASH_FAILED;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("pithy", name) == 0) {
    funcs->options = squash_pithy_options;
    funcs->get_uncompressed_size = squash_pithy_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_pithy_get_max_compressed_size;
    funcs->decompress_buffer = squash_pithy_decompress_buffer;
    funcs->compress_buffer = squash_pithy_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
