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
 *   Evan Nemerson <evan@nemerson.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <new>

#include <squash/squash.h>

#include "doboz/Source/Doboz/Compressor.h"
#include "doboz/Source/Doboz/Decompressor.h"

extern "C" SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static size_t
squash_doboz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return (size_t) doboz::Compressor::getMaxCompressedSize ((uint64_t) uncompressed_length);
}

static size_t
squash_doboz_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  doboz::Decompressor decompressor;
  doboz::CompressionInfo compression_info;
  doboz::Result e;

  e = decompressor.getCompressionInfo (compressed, compressed_length, compression_info);

  return (e == doboz::RESULT_OK) ? compression_info.uncompressedSize : 0;
}

static SquashStatus
squash_doboz_status (doboz::Result status) {
  SquashStatus res = SQUASH_FAILED;

  switch (status) {
    case doboz::RESULT_OK:
      res = SQUASH_OK;
      break;
    case doboz::RESULT_ERROR_BUFFER_TOO_SMALL:
      res = SQUASH_BUFFER_FULL;
      break;
    case doboz::RESULT_ERROR_CORRUPTED_DATA:
      res = SQUASH_FAILED;
      break;
    case doboz::RESULT_ERROR_UNSUPPORTED_VERSION:
      res = SQUASH_FAILED;
      break;
  }

  return res;
}

static SquashStatus
squash_doboz_compress_buffer (SquashCodec* codec,
                              uint8_t* compressed, size_t* compressed_length,
                              const uint8_t* uncompressed, size_t uncompressed_length,
                              SquashOptions* options) {
  doboz::Result doboz_e;
  doboz::Compressor compressor;
  size_t compressed_size;

  try {
    doboz_e = compressor.compress ((void*) uncompressed, uncompressed_length, (void*) compressed, *compressed_length, compressed_size);
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }

  if (doboz_e != doboz::RESULT_OK) {
    return squash_doboz_status (doboz_e);
  }
  *compressed_length = compressed_size;

  return SQUASH_OK;
}

static SquashStatus
squash_doboz_decompress_buffer (SquashCodec* codec,
                                uint8_t* decompressed, size_t* decompressed_length,
                                const uint8_t* compressed, size_t compressed_length,
                                SquashOptions* options) {
  doboz::Result doboz_e;
  doboz::Decompressor decompressor;
  doboz::CompressionInfo compression_info;

  try {
    doboz_e = decompressor.decompress ((void*) compressed, compressed_length, (void*) decompressed, *decompressed_length);
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }

  if (doboz_e != doboz::RESULT_OK) {
    return squash_doboz_status (doboz_e);
  }

  if (decompressor.getCompressionInfo (compressed, compressed_length, compression_info) != doboz::RESULT_OK) {
    return SQUASH_FAILED;
  }
  *decompressed_length = (size_t) compression_info.uncompressedSize;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("doboz", name) == 0) {
    funcs->get_uncompressed_size = squash_doboz_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_doboz_get_max_compressed_size;
    funcs->decompress_buffer = squash_doboz_decompress_buffer;
    funcs->compress_buffer = squash_doboz_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
