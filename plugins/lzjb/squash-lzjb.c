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
#include <errno.h>

#include <squash/squash.h>

#include "lzjb/lzjb.h"

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec       (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_lzjb_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return LZJB_MAX_COMPRESSED_SIZE(uncompressed_size);
}

static SquashStatus
squash_lzjb_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  *compressed_size = lzjb_compress (uncompressed, compressed, uncompressed_size, *compressed_size);
  return SQUASH_LIKELY(*compressed_size != 0) ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

static SquashStatus
squash_lzjb_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  LZJBResult res = lzjb_decompress (compressed, decompressed, compressed_size, decompressed_size);
  switch (res) {
    case LZJB_OK:
      return SQUASH_OK;
    case LZJB_BAD_DATA:
      return SQUASH_FAILED;
    case LZJB_WOULD_OVERFLOW:
      return SQUASH_BUFFER_FULL;
  }
  squash_assert_unreachable ();
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("lzjb", name) == 0)) {
    impl->get_max_compressed_size = squash_lzjb_get_max_compressed_size;
    impl->decompress_buffer = squash_lzjb_decompress_buffer;
    impl->compress_buffer = squash_lzjb_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
