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
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);

static void* squash_bsc_malloc (size_t size) {
  return squash_malloc (size);
}

static void squash_bsc_free (void* ptr) {
  squash_free (ptr);
}

static size_t
squash_bsc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + LIBBSC_HEADER_SIZE;
}

static size_t
squash_bsc_get_uncompressed_size (SquashCodec* codec,
                                  size_t compressed_size,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)]) {
  int p_block_size, p_data_size;

#if INT_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(INT_MAX < compressed_size))
    return (squash_error (SQUASH_RANGE), 0);
#endif

  int res = bsc_block_info (compressed, (int) compressed_size, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (res != LIBBSC_NO_ERROR) {
    return 0;
  } else {
#if SIZE_MAX < INT_MAX
    if (SQUASH_UNLIKELY(SIZE_MAX < p_data_size))
      return (squash_error (SQUASH_RANGE), 0);
#endif
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
squash_bsc_compress_buffer_unsafe (SquashCodec* codec,
                                   size_t* compressed_size,
                                   uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                                   size_t uncompressed_size,
                                   const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                                   SquashOptions* options) {
  const int lzp_hash_size = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_LZP_HASH_SIZE);
  const int lzp_min_len = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_LZP_MIN_LEN);
  const int block_sorter = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_BLOCK_SORTER);
  const int coder = squash_codec_get_option_int_index (codec, options, SQUASH_BSC_OPT_CODER);
  const int features = squash_bsc_options_get_features (codec, options);

#if INT_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(INT_MAX < uncompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  if (SQUASH_UNLIKELY(*compressed_size < (uncompressed_size + LIBBSC_HEADER_SIZE)))
    return squash_error (SQUASH_BUFFER_FULL);

  const int res = bsc_compress (uncompressed, compressed, (int) uncompressed_size,
                                lzp_hash_size, lzp_min_len, block_sorter, coder, features);

  if (SQUASH_UNLIKELY(res < 0)) {
    return squash_error (SQUASH_FAILED);
  }

#if SIZE_MAX < INT_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < res))
    return squash_error (SQUASH_RANGE);
#endif

  *compressed_size = (size_t) res;

  return SQUASH_OK;
}

static SquashStatus
squash_bsc_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_size,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                              size_t compressed_size,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                              SquashOptions* options) {
#if INT_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(INT_MAX < compressed_size) ||
      SQUASH_UNLIKELY(INT_MAX < *decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  const int features = squash_bsc_options_get_features (codec, options);

  int p_block_size, p_data_size;

  int res = bsc_block_info (compressed, (int) compressed_size, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (SQUASH_UNLIKELY(p_block_size != (int) compressed_size))
    return squash_error (SQUASH_FAILED);
  if (SQUASH_UNLIKELY(p_data_size > (int) *decompressed_size))
    return squash_error (SQUASH_BUFFER_FULL);

  res = bsc_decompress (compressed, p_block_size, decompressed, p_data_size, features);

  if (SQUASH_UNLIKELY(res < 0))
    return squash_error (SQUASH_FAILED);

#if SIZE_MAX < INT_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < p_data_size))
    return squash_error (SQUASH_RANGE);
#endif

  *decompressed_size = (size_t) p_data_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  bsc_init_full (LIBBSC_DEFAULT_FEATURES, squash_bsc_malloc, NULL, squash_bsc_free);

  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("bsc", name) == 0)) {
    impl->options = squash_bsc_options;
    impl->get_uncompressed_size = squash_bsc_get_uncompressed_size;
    impl->get_max_compressed_size = squash_bsc_get_max_compressed_size;
    impl->decompress_buffer = squash_bsc_decompress_buffer;
    impl->compress_buffer_unsafe = squash_bsc_compress_buffer_unsafe;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
