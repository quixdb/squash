/* Copyright (c) 2013 The Squash Authors
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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <squash/squash.h>
#include <lzf.h>

#include <stdio.h>

SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_lzf_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  const size_t res =
#if LZF_VERSION >= 0x0106
    LZF_MAX_COMPRESSED_SIZE(uncompressed_length) + 1;
#else
    ((((uncompressed_length) * 33) >> 5 ) + 1) + 1;
#endif
    return (res > 4) ? (res + 2) : 4;
}

static SquashStatus
squash_lzf_decompress_buffer (SquashCodec* codec,
                              uint8_t* decompressed, size_t* decompressed_length,
                              const uint8_t* compressed, size_t compressed_length,
                              SquashOptions* options) {
  SquashStatus res = SQUASH_OK;
  unsigned int lzf_e;

  lzf_e = (size_t) lzf_decompress ((void*) compressed, (unsigned int) compressed_length,
                                   (void*) decompressed, (unsigned int) *decompressed_length);

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
    *decompressed_length = (size_t) lzf_e;
  }

  return res;
}

static SquashStatus
squash_lzf_compress_buffer (SquashCodec* codec,
                            uint8_t* compressed, size_t* compressed_length,
                            const uint8_t* uncompressed, size_t uncompressed_length,
                            SquashOptions* options) {
  unsigned int lzf_e;

  lzf_e = lzf_compress ((void*) uncompressed, (unsigned int) uncompressed_length,
                        (void*) compressed, (unsigned int) *compressed_length);

  if (lzf_e == 0) {
    return SQUASH_BUFFER_FULL;
  } else {
    *compressed_length = (size_t) lzf_e;
  }

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzf", name) == 0) {
    funcs->get_max_compressed_size = squash_lzf_get_max_compressed_size;
    funcs->decompress_buffer = squash_lzf_decompress_buffer;
    funcs->compress_buffer = squash_lzf_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
