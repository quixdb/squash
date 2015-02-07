/* Copyright (c) 2015 The Squash Authors
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

#include "ms-compress/compression.h"

SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static CompressionFormat
squash_codec_to_ms_compress_format (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);

  if (name[5] == 0)
    return COMPRESSION_LZNT1;
  else if (name[6] == 0)
    return COMPRESSION_XPRESS;
  else if (name[14] == 0)
    return COMPRESSION_XPRESS_HUFF;
  else
    assert (false);
}

static SquashStatus
squash_ms_compress_compress_buffer (SquashCodec* codec,
                                    uint8_t* compressed, size_t* compressed_length,
                                    const uint8_t* uncompressed, size_t uncompressed_length,
                                    SquashOptions* options) {
  switch (squash_codec_to_ms_compress_format (codec)) {
    case COMPRESSION_LZNT1:
      *compressed_length = lznt1_compress ((const_bytes) uncompressed, uncompressed_length, (bytes) compressed, *compressed_length);
      break;
    case COMPRESSION_XPRESS:
      *compressed_length = xpress_compress ((const_bytes) uncompressed, uncompressed_length, (bytes) compressed, *compressed_length);
      break;
    case COMPRESSION_XPRESS_HUFF:
      *compressed_length = xpress_huff_compress ((const_bytes) uncompressed, uncompressed_length, (bytes) compressed, *compressed_length);
      break;
  }

  return SQUASH_OK;
}

static SquashStatus
squash_ms_compress_decompress_buffer (SquashCodec* codec,
                                      uint8_t* decompressed, size_t* decompressed_length,
                                      const uint8_t* compressed, size_t compressed_length,
                                      SquashOptions* options) {
  switch (squash_codec_to_ms_compress_format (codec)) {
    case COMPRESSION_LZNT1:
      *decompressed_length = lznt1_decompress ((const_bytes) compressed, compressed_length, (bytes) decompressed, *decompressed_length);
      break;
    case COMPRESSION_XPRESS:
      *decompressed_length = xpress_decompress ((const_bytes) compressed, compressed_length, (bytes) decompressed, *decompressed_length);
      break;
    case COMPRESSION_XPRESS_HUFF:
      *decompressed_length = xpress_huff_decompress ((const_bytes) compressed, compressed_length, (bytes) decompressed, *decompressed_length);
      break;
  }

  return SQUASH_OK;
}

static size_t
squash_ms_compress_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  switch (squash_codec_to_ms_compress_format (codec)) {
    case COMPRESSION_LZNT1:
      return lznt1_max_compressed_size (uncompressed_length);
      break;
    case COMPRESSION_XPRESS:
      return xpress_max_compressed_size (uncompressed_length) + 4;
      break;
    case COMPRESSION_XPRESS_HUFF:
      return uncompressed_length + 384 + (uncompressed_length / 100);
      break;
    default:
      assert (false);
  }
}

static size_t
squash_ms_compress_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  switch (squash_codec_to_ms_compress_format (codec)) {
    case COMPRESSION_LZNT1:
      return lznt1_uncompressed_size ((const_bytes) compressed, compressed_length);
      break;
    case COMPRESSION_XPRESS:
      return xpress_uncompressed_size ((const_bytes) compressed, compressed_length);
      break;
    default:
      assert (false);
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lznt1", name) == 0 ||
      strcmp ("xpress", name) == 0 ||
      strcmp ("xpress-huffman", name) == 0) {
    funcs->get_max_compressed_size = squash_ms_compress_get_max_compressed_size;
    funcs->decompress_buffer = squash_ms_compress_decompress_buffer;
    funcs->compress_buffer = squash_ms_compress_compress_buffer;
    if (strcmp ("xpress-huffman", name) != 0)
      funcs->get_uncompressed_size = squash_ms_compress_get_uncompressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
