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
#include <lzf.h>

/* No header :( */
#include <lzf_c_best.c>

enum SquashLzfOptIndex {
  SQUASH_LZF_OPT_LEVEL = 0,
};

static SquashOptionInfo squash_lzf_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_ENUM_INT,
    .info.enum_int = {
      2, (const int []) { 1, 9 } },
    .default_value.int_value = 1 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_lzf_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  const size_t res =
#if LZF_VERSION >= 0x0106
    LZF_MAX_COMPRESSED_SIZE(uncompressed_size) + 1;
#else
    ((((uncompressed_size) * 33) >> 5 ) + 1) + 1;
#endif
    return (res > 4) ? (res + 2) : 4;
}

static SquashStatus
squash_lzf_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_size,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                              size_t compressed_size,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                              SquashOptions* options) {
  SquashStatus res = SQUASH_OK;
  unsigned int lzf_e;

#if UINT_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(UINT_MAX < compressed_size) ||
      SQUASH_UNLIKELY(UINT_MAX < *decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  lzf_e = (size_t) lzf_decompress ((void*) compressed, (unsigned int) compressed_size,
                                   (void*) decompressed, (unsigned int) *decompressed_size);

  if (lzf_e == 0) {
    switch (errno) {
      case E2BIG:
        res = SQUASH_BUFFER_FULL;
        break;
      case EINVAL:
        res = SQUASH_BAD_VALUE;
        break;
      default:
        res = SQUASH_FAILED;
        break;
    }
  } else {
#if SIZE_MAX < UINT_MAX
    if (SQUASH_UNLIKELY(SIZE_MAX < lzf_e))
      return squash_error (SQUASH_RANGE);
#endif
    *decompressed_size = (size_t) lzf_e;
  }

  return res;
}

static SquashStatus
squash_lzf_compress_buffer (SquashCodec* codec,
                            size_t* compressed_size,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                            size_t uncompressed_size,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                            SquashOptions* options) {
  unsigned int lzf_e;
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_LZF_OPT_LEVEL);

#if UINT_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(UINT_MAX < uncompressed_size) ||
      SQUASH_UNLIKELY(UINT_MAX < *compressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  if (uncompressed_size == 1) {
    const uint8_t buf[2] = { uncompressed[0], 0x00 };

    if (level == 1)
      lzf_e = lzf_compress ((void*) buf, (unsigned int) uncompressed_size,
                            (void*) compressed, (unsigned int) *compressed_size);
    else
      lzf_e = lzf_compress_best ((void*) buf, (unsigned int) uncompressed_size,
                                 (void*) compressed, (unsigned int) *compressed_size);
  } else {
    if (level == 1)
      lzf_e = lzf_compress ((void*) uncompressed, (unsigned int) uncompressed_size,
                            (void*) compressed, (unsigned int) *compressed_size);
    else
      lzf_e = lzf_compress_best ((void*) uncompressed, (unsigned int) uncompressed_size,
                                 (void*) compressed, (unsigned int) *compressed_size);
  }

  if (lzf_e == 0) {
    return SQUASH_BUFFER_FULL;
  } else {
#if SIZE_MAX < UINT_MAX
    if (SQUASH_UNLIKELY(SIZE_MAX < lzf_e))
      return squash_error (SQUASH_RANGE);
#endif
    *compressed_size = (size_t) lzf_e;
  }

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzf", name) == 0) {
    impl->options = squash_lzf_options;
    impl->get_max_compressed_size = squash_lzf_get_max_compressed_size;
    impl->decompress_buffer = squash_lzf_decompress_buffer;
    impl->compress_buffer = squash_lzf_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
