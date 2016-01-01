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

#include "FastARI/FastAri.h"

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_fari_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 8 + (uncompressed_size / 34);
}

static SquashStatus
squash_fari_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  SquashContext* ctx = squash_codec_get_context (codec);
  void* workmem = squash_malloc (ctx, FA_WORKMEM);
  int fari_e = (size_t) fa_decompress ((const unsigned char*) compressed, (unsigned char*) decompressed,
                                       compressed_size, decompressed_size, workmem);
  squash_free (ctx, workmem);

  switch (fari_e) {
    case 0:
      return SQUASH_OK;
      break;
    case 1:
      return squash_error (SQUASH_MEMORY);
      break;
    case 2:
      return squash_error (SQUASH_BUFFER_FULL);
      break;
    default:
      return squash_error (SQUASH_FAILED);
      break;
  }
}

static SquashStatus
squash_fari_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  SquashContext* ctx = squash_codec_get_context (codec);
  void* workmem = squash_malloc (ctx, FA_WORKMEM);
  int fari_e = fa_compress ((const unsigned char*) uncompressed, (unsigned char*) compressed,
                            uncompressed_size, compressed_size, workmem);
  squash_free (ctx, workmem);

  return SQUASH_LIKELY(fari_e == 0) ? SQUASH_OK : SQUASH_FAILED;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("fari", name) == 0)) {
    impl->get_max_compressed_size = squash_fari_get_max_compressed_size;
    impl->decompress_buffer = squash_fari_decompress_buffer;
    impl->compress_buffer_unsafe = squash_fari_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
