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

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_doboz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return (size_t) doboz::Compressor::getMaxCompressedSize ((uint64_t) uncompressed_size);
}

static size_t
squash_doboz_get_uncompressed_size (SquashCodec* codec,
                                    size_t compressed_size,
                                    const uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)]) {
  doboz::Decompressor decompressor;
  doboz::CompressionInfo compression_info;
  doboz::Result e;

  e = decompressor.getCompressionInfo (compressed, compressed_size, compression_info);

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
                              size_t* compressed_size,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  doboz::Result doboz_e;
  doboz::Compressor compressor;
  size_t compressed_size_out;

  try {
    doboz_e = compressor.compress ((void*) uncompressed, uncompressed_size, (void*) compressed, *compressed_size, compressed_size_out);
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }

  if (doboz_e != doboz::RESULT_OK) {
    return squash_doboz_status (doboz_e);
  }
  *compressed_size = compressed_size_out;

  return SQUASH_OK;
}

static SquashStatus
squash_doboz_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                SquashOptions* options) {
  doboz::Result doboz_e;
  doboz::Decompressor decompressor;
  doboz::CompressionInfo compression_info;

  try {
    doboz_e = decompressor.decompress ((void*) compressed, compressed_size, (void*) decompressed, *decompressed_size);
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }

  if (doboz_e != doboz::RESULT_OK) {
    return squash_doboz_status (doboz_e);
  }

  if (decompressor.getCompressionInfo (compressed, compressed_size, compression_info) != doboz::RESULT_OK) {
    return SQUASH_FAILED;
  }
  *decompressed_size = (size_t) compression_info.uncompressedSize;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("doboz", name) == 0) {
    impl->get_uncompressed_size = squash_doboz_get_uncompressed_size;
    impl->get_max_compressed_size = squash_doboz_get_max_compressed_size;
    impl->decompress_buffer = squash_doboz_decompress_buffer;
    impl->compress_buffer = squash_doboz_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
