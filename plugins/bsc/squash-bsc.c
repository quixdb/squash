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
#include <limits.h>

#include <squash/squash.h>

#include "libbsc/libbsc/libbsc.h"

enum SquashBscOptIndex {
  SQUASH_BSC_OPT_FAST_MODE = 0,
  SQUASH_BSC_OPT_MULTI_THREADING,
  SQUASH_BSC_OPT_LARGE_PAGES,
  SQUASH_BSC_OPT_CUDA,
  SQUASH_BSC_OPT_LZP_HASH_SIZE,
  SQUASH_BSC_OPT_LZP_MIN_LEN,
  SQUASH_BSC_OPT_BLOCK_SORTER,
  SQUASH_BSC_OPT_CODER
};

static SquashOptionInfo squash_bsc_options[] = {
  { "fast-mode",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = true },
  { "multi-threading",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = true },
  { "large-pages",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { "cuda",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { "lzp-hash-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 10,
      .max = 28 },
    .default_value.int_value = 16 },
  { "lzp-min-len",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 4,
      .max = 255 },
    .default_value.int_value = 128 },
  { "block-sorter",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "none", LIBBSC_BLOCKSORTER_NONE },
        { "bwt", LIBBSC_BLOCKSORTER_BWT },
        { NULL, 0 } } },
    .default_value.int_value = LIBBSC_BLOCKSORTER_BWT },
  { "coder",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "none", LIBBSC_CODER_NONE },
        { "qflc-static", LIBBSC_CODER_QLFC_STATIC },
        { "qflc-adaptive", LIBBSC_CODER_QLFC_ADAPTIVE },
        { NULL, 0 } } },
    .default_value.int_value = LIBBSC_CODER_QLFC_STATIC },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_bsc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + LIBBSC_HEADER_SIZE;
}

static size_t
squash_lzg_get_uncompressed_size (SquashCodec* codec,
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  int p_block_size, p_data_size;

  int res = bsc_block_info (compressed, (int) compressed_length, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (res != LIBBSC_NO_ERROR) {
    return 0;
  } else {
    return (size_t) p_data_size;
  }
}

static int
squash_bsc_options_get_features (SquashCodec* codec,
                                 SquashOptions* options) {
  return
    (squash_codec_get_option_bool_index (codec, options, SQUASH_BSC_OPT_FAST_MODE) ? LIBBSC_FEATURE_FASTMODE : 0) |
    (squash_codec_get_option_bool_index (codec, options, SQUASH_BSC_OPT_MULTI_THREADING) ? LIBBSC_FEATURE_MULTITHREADING : 0) |
    (squash_codec_get_option_bool_index (codec, options, SQUASH_BSC_OPT_LARGE_PAGES) ? LIBBSC_FEATURE_LARGEPAGES : 0) |
    (squash_codec_get_option_bool_index (codec, options, SQUASH_BSC_OPT_CUDA) ? LIBBSC_FEATURE_CUDA : 0);
}

static SquashStatus
squash_bsc_compress_buffer (SquashCodec* codec,
                            size_t* compressed_length,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOptions* options) {
  int lzp_hash_size = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_LZP_HASH_SIZE);
  int lzp_min_len = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_LZP_MIN_LEN);
  int block_sorter = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_BLOCK_SORTER);
  int coder = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_CODER);
  int features = squash_bsc_options_get_features (codec, options);

  if (*compressed_length < (uncompressed_length + LIBBSC_HEADER_SIZE))
    return squash_error (SQUASH_BUFFER_FULL);

  int res = bsc_compress (uncompressed, compressed, (int) uncompressed_length,
                          lzp_hash_size, lzp_min_len, block_sorter, coder, features);

  if (res < 0) {
    return squash_error (SQUASH_FAILED);
  }

  *compressed_length = (size_t) res;

  return SQUASH_OK;
}

static SquashStatus
squash_bsc_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_length,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                              size_t compressed_length,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                              SquashOptions* options) {
  assert (compressed_length < (size_t) INT_MAX);
  assert (*decompressed_length < (size_t) INT_MAX);

  int features = squash_bsc_options_get_features (codec, options);

  int p_block_size, p_data_size;

  int res = bsc_block_info (compressed, (int) compressed_length, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (p_block_size != (int) compressed_length)
    return squash_error (SQUASH_FAILED);
  if (p_data_size > (int) *decompressed_length)
    return squash_error (SQUASH_BUFFER_FULL);

  res = bsc_decompress (compressed, p_block_size, decompressed, p_data_size, features);

  if (res < 0)
    return squash_error (SQUASH_FAILED);

  *decompressed_length = (size_t) p_data_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  bsc_init (LIBBSC_DEFAULT_FEATURES);

  const char* name = squash_codec_get_name (codec);

  if (strcmp ("bsc", name) == 0) {
    funcs->options = squash_bsc_options;
    funcs->get_uncompressed_size = squash_lzg_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_bsc_get_max_compressed_size;
    funcs->decompress_buffer = squash_bsc_decompress_buffer;
    funcs->compress_buffer = squash_bsc_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
