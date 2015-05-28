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
#include <lz4.h>
#include <lz4hc.h>

enum SquashLZ4OptIndex {
  SQUASH_LZ4_OPT_LEVEL = 0
};

static SquashOptionInfo squash_lz4_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 7,
      .max = 14 },
    .default_value.int_value = 7 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SquashStatus             squash_plugin_init_lz4f    (SquashCodec* codec, SquashCodecFuncs* funcs);

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_lz4_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return LZ4_COMPRESSBOUND(uncompressed_length);
}

static SquashStatus
squash_lz4_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_length,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                              size_t compressed_length,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                              SquashOptions* options) {
  int lz4_e = LZ4_decompress_safe ((char*) compressed,
                                   (char*) decompressed,
                                   (int) compressed_length,
                                   (int) *decompressed_length);

  if (lz4_e < 0) {
    return SQUASH_FAILED;
  } else {
    *decompressed_length = (size_t) lz4_e;
    return SQUASH_OK;
  }
}

/* static int */
/* squash_lz4_level_to_fast_mode (const int level) { */
/*     switch (level) { */
/*       case 1: */
/*         return 32; */
/*       case 2: */
/*         return 24; */
/*       case 3: */
/*         return 17; */
/*       case 4: */
/*         return 8; */
/*       case 5: */
/*         return 4; */
/*       case 6: */
/*         return 2; */
/*       default: */
/*         squash_assert_unreachable(); */
/*     } */
/* } */

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
      squash_assert_unreachable();
  }
}

static SquashStatus
squash_lz4_compress_buffer (SquashCodec* codec,
                            size_t* compressed_length,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOptions* options) {
  int level = squash_codec_get_option_int_index (codec, options, SQUASH_LZ4_OPT_LEVEL);

  if (level == 7) {
    *compressed_length = LZ4_compress_limitedOutput ((char*) uncompressed,
                                                     (char*) compressed,
                                                     (int) uncompressed_length,
                                                     (int) *compressed_length);
  } else if (level < 7) {
    /* To be added when the fastMode branch is merged into LZ4 */
    /* *compressed_length = LZ4_compress_fast ((const char*) uncompressed, */
    /*                                         (char*) compressed, */
    /*                                         (int) uncompressed_length, */
    /*                                         squash_lz4_level_to_fast_mode (level)); */
    squash_assert_unreachable();
  } else if (level < 17) {
    *compressed_length = LZ4_compressHC2_limitedOutput ((char*) uncompressed,
                                                        (char*) compressed,
                                                        (int) uncompressed_length,
                                                        (int) *compressed_length,
                                                        squash_lz4_level_to_hc_level (level));
  } else {
    squash_assert_unreachable();
  }

  return (*compressed_length == 0) ? SQUASH_BUFFER_FULL : SQUASH_OK;
}

static SquashStatus
squash_lz4_compress_buffer_unsafe (SquashCodec* codec,
                                   size_t* compressed_length,
                                   uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                   size_t uncompressed_length,
                                   const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                   SquashOptions* options) {
  int level = squash_codec_get_option_int_index (codec, options, SQUASH_LZ4_OPT_LEVEL);

  assert (*compressed_length >= LZ4_COMPRESSBOUND(uncompressed_length));

  if (level == 7) {
    *compressed_length = LZ4_compress ((char*) uncompressed,
                                       (char*) compressed,
                                       (int) uncompressed_length);
  } else if (level < 7) {
    // No LZ4_compress_fast_limitedOutput?
    /* To be added when the fastMode branch is merged into LZ4 */
    /* *compressed_length = LZ4_compress_fast ((const char*) uncompressed, */
    /*                                         (char*) compressed, */
    /*                                         (int) uncompressed_length, */
    /*                                         squash_lz4_level_to_fast_mode (level)); */
    squash_assert_unreachable();
  } else {
    *compressed_length = LZ4_compressHC2 ((char*) uncompressed,
                                          (char*) compressed,
                                          (int) uncompressed_length,
                                          squash_lz4_level_to_hc_level (level));
  }

  return (*compressed_length == 0) ? SQUASH_BUFFER_FULL : SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lz4", name) == 0) {
    funcs->options = squash_lz4_options;
    funcs->get_max_compressed_size = squash_lz4_get_max_compressed_size;
    funcs->decompress_buffer = squash_lz4_decompress_buffer;
    funcs->compress_buffer = squash_lz4_compress_buffer;
    funcs->compress_buffer_unsafe = squash_lz4_compress_buffer_unsafe;
  } else {
    return squash_plugin_init_lz4f (codec, funcs);
  }

  return SQUASH_OK;
}
