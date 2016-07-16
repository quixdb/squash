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
#include <limits.h>

#include <squash/squash.h>
#include <lz4.h>
#include <lz4hc.h>

enum SquashLZ4OptIndex {
  SQUASH_LZ4_OPT_LEVEL = 0
};

static SquashOptionInfo squash_lz4_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 14 },
    .default_value.int_value = 7 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SquashStatus             squash_plugin_init_lz4f    (SquashCodec* codec, SquashCodecImpl* impl);

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_lz4_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return LZ4_COMPRESSBOUND(uncompressed_size);
}

static SquashStatus
squash_lz4_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_size,
                              uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                              size_t compressed_size,
                              const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                              SquashOptions* options) {
#if INT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(INT_MAX < compressed_size) ||
      HEDLEY_UNLIKELY(INT_MAX < *decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  int lz4_e = LZ4_decompress_safe ((char*) compressed,
                                   (char*) decompressed,
                                   (int) compressed_size,
                                   (int) *decompressed_size);

  if (lz4_e < 0) {
    return SQUASH_FAILED;
  } else {
#if SIZE_MAX < INT_MAX
    if (HEDLEY_UNLIKELY(SIZE_MAX < lz4_e))
      return squash_error (SQUASH_RANGE);
#endif
    *decompressed_size = (size_t) lz4_e;
    return SQUASH_OK;
  }
}

static int
squash_lz4_level_to_fast_mode (const int level) {
    switch (level) {
      case 1:
        return 32;
      case 2:
        return 24;
      case 3:
        return 17;
      case 4:
        return 8;
      case 5:
        return 4;
      case 6:
        return 2;
      default:
        HEDLEY_UNREACHABLE();
    }
}

static int
squash_lz4_level_to_hc_level (const int level) {
  switch (level) {
    case 8:
      return 2;
    case 9:
      return 4;
    case 10:
      return 6;
    case 11:
      return 9;
    case 12:
      return 12;
    case 13:
      return 14;
    case 14:
      return 16;
    default:
      HEDLEY_UNREACHABLE();
  }
}

static SquashStatus
squash_lz4_compress_buffer (SquashCodec* codec,
                            size_t* compressed_size,
                            uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                            size_t uncompressed_size,
                            const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                            SquashOptions* options) {
  int level = squash_options_get_int_at (options, codec, SQUASH_LZ4_OPT_LEVEL);

#if INT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(INT_MAX < uncompressed_size) ||
      HEDLEY_UNLIKELY(INT_MAX < *compressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  int lz4_r;

  if (level == 7) {
    lz4_r = LZ4_compress_limitedOutput ((char*) uncompressed,
                                        (char*) compressed,
                                        (int) uncompressed_size,
                                        (int) *compressed_size);
  } else if (level < 7) {
    lz4_r = LZ4_compress_fast ((const char*) uncompressed,
                               (char*) compressed,
                               (int) uncompressed_size,
                               (int) *compressed_size,
                               squash_lz4_level_to_fast_mode (level));
  } else if (level < 17) {
    lz4_r = LZ4_compressHC2_limitedOutput ((char*) uncompressed,
                                           (char*) compressed,
                                           (int) uncompressed_size,
                                           (int) *compressed_size,
                                           squash_lz4_level_to_hc_level (level));
  } else {
    HEDLEY_UNREACHABLE();
  }

#if SIZE_MAX < INT_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < lz4_r))
    return squash_error (SQUASH_RANGE);
#endif

  *compressed_size = lz4_r;

  return HEDLEY_UNLIKELY(lz4_r == 0) ? squash_error (SQUASH_BUFFER_FULL) : SQUASH_OK;
}

static SquashStatus
squash_lz4_compress_buffer_unsafe (SquashCodec* codec,
                                   size_t* compressed_size,
                                   uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                                   size_t uncompressed_size,
                                   const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                                   SquashOptions* options) {
  int level = squash_options_get_int_at (options, codec, SQUASH_LZ4_OPT_LEVEL);

#if INT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(INT_MAX < uncompressed_size) ||
      HEDLEY_UNLIKELY(INT_MAX < *compressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  assert (*compressed_size >= LZ4_COMPRESSBOUND(uncompressed_size));

  int lz4_r;

  if (level == 7) {
    lz4_r = LZ4_compress ((char*) uncompressed,
                          (char*) compressed,
                          (int) uncompressed_size);
  } else if (level < 7) {
    lz4_r = LZ4_compress_fast ((const char*) uncompressed,
                               (char*) compressed,
                               (int) uncompressed_size,
                               (int) *compressed_size,
                               squash_lz4_level_to_fast_mode (level));
  } else {
    lz4_r = LZ4_compressHC2 ((char*) uncompressed,
                             (char*) compressed,
                             (int) uncompressed_size,
                             squash_lz4_level_to_hc_level (level));
  }

#if SIZE_MAX < INT_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < lz4_r))
    return squash_error (SQUASH_RANGE);
#endif

  *compressed_size = lz4_r;

  return (lz4_r == 0) ? SQUASH_BUFFER_FULL : SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lz4-raw", name) == 0) {
    impl->options = squash_lz4_options;
    impl->get_max_compressed_size = squash_lz4_get_max_compressed_size;
    impl->decompress_buffer = squash_lz4_decompress_buffer;
    impl->compress_buffer = squash_lz4_compress_buffer;
    impl->compress_buffer_unsafe = squash_lz4_compress_buffer_unsafe;
  } else {
    return squash_plugin_init_lz4f (codec, impl);
  }

  return SQUASH_OK;
}
