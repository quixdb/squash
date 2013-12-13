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

#include "FastARI/FastAri.h"

SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_fari_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + 4 + (uncompressed_length / 50);
}

static SquashStatus
squash_fari_decompress_buffer (SquashCodec* codec,
                               uint8_t* decompressed, size_t* decompressed_length,
                               const uint8_t* compressed, size_t compressed_length,
                               SquashOptions* options) {
  size_t orig_decompressed_length = *decompressed_length;

  int fari_e = (size_t) fa_decompress ((const unsigned char*) compressed, (unsigned char*) decompressed,
                                       compressed_length, decompressed_length);

  switch (fari_e) {
    case 0:
      return SQUASH_OK;
      break;
    case 1:
      return SQUASH_MEMORY;
      break;
    case 2:
      return SQUASH_BUFFER_FULL;
      break;
    default:
      return SQUASH_FAILED;
      break;
  }
}

static SquashStatus
squash_fari_compress_buffer (SquashCodec* codec,
                             uint8_t* compressed, size_t* compressed_length,
                             const uint8_t* uncompressed, size_t uncompressed_length,
                             SquashOptions* options) {
  int fari_e = fa_compress ((const unsigned char*) uncompressed, (unsigned char*) compressed,
                            uncompressed_length, compressed_length);

  return fari_e == 0 ? SQUASH_OK : SQUASH_FAILED;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("fari", name) == 0) {
    funcs->get_max_compressed_size = squash_fari_get_max_compressed_size;
    funcs->decompress_buffer = squash_fari_decompress_buffer;
    funcs->compress_buffer = squash_fari_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
