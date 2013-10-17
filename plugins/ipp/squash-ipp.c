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

#include <squash/squash.h>

#include <ippdc.h>

SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_ipp_lzo_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + uncompressed_length / 16 + 64 + 3;
}

static SquashStatus
squash_ipp_lzo_decompress_buffer (SquashCodec* codec,
                                  uint8_t* decompressed, size_t* decompressed_length,
                                  const uint8_t* compressed, size_t compressed_length,
                                  SquashOptions* options) {
  Ipp32u lzo_decompressed_length = (Ipp32u) *decompressed_length;
  IppStatus lzo_e;

  lzo_e = ippsDecodeLZOSafe_8u (compressed, (size_t) compressed_length, decompressed, &lzo_decompressed_length);
  switch (lzo_e) {
  case ippStsNoErr:
    break;
  case ippStsDstSizeLessExpected:
    return SQUASH_BUFFER_EMPTY;
  default:
    return SQUASH_FAILED;
  }
  *decompressed_length = lzo_decompressed_length;

  return SQUASH_OK;
}

static SquashStatus
squash_ipp_lzo_compress_buffer (SquashCodec* codec,
                                uint8_t* compressed, size_t* compressed_length,
                                const uint8_t* uncompressed, size_t uncompressed_length,
                                SquashOptions* options) {
  IppLZOState_8u *lzo_state;
  Ipp32u lzo_size;
  Ipp32u lzo_compressed_length;

  ippsEncodeLZOGetSize (IppLZO1XST, uncompressed_length, &lzo_size);
  lzo_state = (IppLZOState_8u*) malloc (lzo_size);
  ippsEncodeLZOInit_8u (IppLZO1XST, uncompressed_length, lzo_state);

  ippsEncodeLZO_8u (uncompressed, uncompressed_length,
                    compressed, &lzo_compressed_length,
                    lzo_state);
  *compressed_length = lzo_compressed_length;

  free (lzo_state);

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzo1x", name) == 0) {
    funcs->get_max_compressed_size = squash_ipp_lzo_get_max_compressed_size;
    funcs->decompress_buffer = squash_ipp_lzo_decompress_buffer;
    funcs->compress_buffer = squash_ipp_lzo_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
