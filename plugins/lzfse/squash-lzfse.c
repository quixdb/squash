/* Copyright (c) 2016 The Squash Authors
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

#include "lzfse/src/lzfse.h"
#include "lzfse/src/lzfse_internal.h"

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_lzfse_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 12;
}

static SquashStatus
squash_lzfse_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                SquashOptions* options) {
  const size_t workmem_size = lzfse_decode_scratch_size ();
  lzfse_decoder_state* ctx = squash_calloc (workmem_size, 1);
  if (SQUASH_UNLIKELY(ctx == NULL))
    return squash_error (SQUASH_FAILED);

  ctx->src_begin = ctx->src = compressed;
  ctx->src_end = compressed + compressed_size;
  ctx->dst_begin = ctx->dst = decompressed;
  ctx->dst_end = decompressed + *decompressed_size;

  const int lret = lzfse_decode (ctx);
  const size_t written = (size_t) (ctx->dst - decompressed);

  squash_free (ctx);

  switch (lret) {
    case LZFSE_STATUS_OK:
      *decompressed_size = written;
      return SQUASH_OK;
    case LZFSE_STATUS_DST_FULL:
      return squash_error (SQUASH_BUFFER_FULL);
    case LZFSE_STATUS_ERROR:
    default:
      return squash_error (SQUASH_FAILED);
  }
}

static SquashStatus
squash_lzfse_compress_buffer (SquashCodec* codec,
                              size_t* compressed_size,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  void* workmem = squash_malloc (lzfse_encode_scratch_size ());
  if (SQUASH_UNLIKELY(workmem == NULL))
    return squash_error (SQUASH_FAILED);

  const size_t r = lzfse_encode_buffer (compressed, *compressed_size,
                                        uncompressed, uncompressed_size,
                                        workmem);

  squash_free (workmem);

  if (SQUASH_UNLIKELY(r == 0))
    return squash_error (SQUASH_BUFFER_FULL);

  *compressed_size = r;
  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("lzfse", name) == 0)) {
    impl->get_max_compressed_size = squash_lzfse_get_max_compressed_size;
    impl->decompress_buffer = squash_lzfse_decompress_buffer;
    impl->compress_buffer = squash_lzfse_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
