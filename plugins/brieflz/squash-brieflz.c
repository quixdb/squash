/* Copyright (c) 2015-2020 The Squash Authors
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
 *   Joergen Ibsen
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <squash/squash.h>
#include <brieflz.h>

enum SquashBriefLZOptIndex {
  SQUASH_BRIEFLZ_OPT_LEVEL = 0,
  SQUASH_BRIEFLZ_OPT_OPTIMAL
};

static SquashOptionInfo squash_brieflz_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 1 },
  { "optimal",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};


SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_brieflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
#if ULONG_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(ULONG_MAX < uncompressed_size))
    return (squash_error (SQUASH_RANGE), 0);
#endif

  const unsigned long r = blz_max_packed_size ((unsigned long) uncompressed_size);

#if SIZE_MAX < ULONG_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < r))
    return (squash_error (SQUASH_RANGE), 0);
#endif

  return (size_t) r;
}

static SquashStatus
squash_brieflz_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_size,
                                  uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                                  size_t compressed_size,
                                  const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                                  SquashOptions* options) {
  const uint8_t *src = compressed;
  unsigned long size;

#if ULONG_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(ULONG_MAX < compressed_size) ||
      HEDLEY_UNLIKELY(ULONG_MAX < *decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  size = blz_depack_safe (src, (unsigned long) compressed_size,
                          decompressed, (unsigned long) *decompressed_size);

  if (HEDLEY_UNLIKELY(size != *decompressed_size))
    return squash_error (SQUASH_FAILED);

#if SIZE_MAX < ULONG_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < size))
    return squash_error (SQUASH_RANGE);
#endif

  *decompressed_size = (size_t) size;

  return SQUASH_OK;
}

static SquashStatus
squash_brieflz_compress_buffer (SquashCodec* codec,
                                size_t* compressed_size,
                                uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                                size_t uncompressed_size,
                                const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                                SquashOptions* options) {
  uint8_t *dst = compressed;
  void *workmem = NULL;
  unsigned long size;
  int level = squash_options_get_bool_at (options, codec, SQUASH_BRIEFLZ_OPT_OPTIMAL) ? 10
            : squash_options_get_int_at (options, codec, SQUASH_BRIEFLZ_OPT_LEVEL);

#if ULONG_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(ULONG_MAX < uncompressed_size) ||
      HEDLEY_UNLIKELY(ULONG_MAX < *compressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  if (HEDLEY_UNLIKELY((unsigned long) *compressed_size
                      < squash_brieflz_get_max_compressed_size (codec, uncompressed_size))) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  workmem = squash_malloc (blz_workmem_size_level ((unsigned long) uncompressed_size, level));

  if (HEDLEY_UNLIKELY(workmem == NULL)) {
    return squash_error (SQUASH_MEMORY);
  }

  size = blz_pack_level (uncompressed, dst,
                         (unsigned long) uncompressed_size,
                         workmem, level);

  squash_free (workmem);

#if SIZE_MAX < ULONG_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < size))
    return squash_error (SQUASH_RANGE);
#endif

  *compressed_size = (size_t) size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("brieflz", name) == 0)) {
    impl->options = squash_brieflz_options;
    impl->info = SQUASH_CODEC_INFO_WRAP_SIZE;
    /* impl->get_uncompressed_size = squash_brieflz_get_uncompressed_size; */
    impl->get_max_compressed_size = squash_brieflz_get_max_compressed_size;
    impl->decompress_buffer = squash_brieflz_decompress_buffer;
    impl->compress_buffer_unsafe = squash_brieflz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
