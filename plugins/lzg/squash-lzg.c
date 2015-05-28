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

#include <squash/squash.h>

#include "liblzg/src/include/lzg.h"

enum SquashLzgOptIndex {
  SQUASH_LZG_OPT_LEVEL = 0,
  SQUASH_LZG_OPT_FAST
};

static SquashOptionInfo squash_lzg_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 5 },
  { "fast",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = true },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

const lzg_encoder_config_t squash_lzg_default_config = {
  LZG_LEVEL_DEFAULT,
  LZG_TRUE,
  NULL,
  NULL
};

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_lzg_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return (size_t) LZG_MaxEncodedSize ((lzg_uint32_t) uncompressed_length);
}

static size_t
squash_lzg_get_uncompressed_size (SquashCodec* codec,
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  return (size_t) LZG_DecodedSize ((const unsigned char*) compressed, (lzg_uint32_t) compressed_length);
}

static SquashStatus
squash_lzg_compress_buffer (SquashCodec* codec,
                            size_t* compressed_length,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOptions* options) {
  lzg_encoder_config_t cfg = {
    squash_codec_get_option_int_index (codec, options, SQUASH_LZG_OPT_LEVEL),
    squash_codec_get_option_bool_index (codec, options, SQUASH_LZG_OPT_FAST),
    NULL,
    NULL
  };

  lzg_uint32_t res = LZG_Encode ((const unsigned char*) uncompressed, (lzg_uint32_t) uncompressed_length,
                                 (unsigned char*) compressed, (lzg_uint32_t) *compressed_length,
                                 &cfg);

  if (res == 0) {
    return squash_error (SQUASH_FAILED);
  } else {
    *compressed_length = (size_t) res;
    return SQUASH_OK;
  }
}

static SquashStatus
squash_lzg_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_length,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                              size_t compressed_length,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                              SquashOptions* options) {
  lzg_uint32_t res = LZG_Decode ((const unsigned char*) compressed, (lzg_uint32_t) compressed_length,
                                 (unsigned char*) decompressed, (lzg_uint32_t) *decompressed_length);

  if (res == 0) {
    return squash_error (SQUASH_FAILED);
  } else {
    *decompressed_length = (size_t) res;
    return SQUASH_OK;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzg", name) == 0) {
    funcs->options = squash_lzg_options;
    funcs->get_uncompressed_size = squash_lzg_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_lzg_get_max_compressed_size;
    funcs->decompress_buffer = squash_lzg_decompress_buffer;
    funcs->compress_buffer = squash_lzg_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
