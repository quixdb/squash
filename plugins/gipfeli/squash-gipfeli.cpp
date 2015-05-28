/* Copyright (c) 2013-2014 The Squash Authors
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

#include <squash/squash.h>

#include "gipfeli/gipfeli.h"

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_gipfeli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  size_t ret;
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();

  ret = compressor->MaxCompressedLength (uncompressed_length);

  delete compressor;

  return ret;
}

static size_t
squash_gipfeli_get_uncompressed_size (SquashCodec* codec,
                                      size_t compressed_length,
                                      const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();
  std::string compressed_str((const char*) compressed, compressed_length);
  size_t uncompressed_length = 0;

  if (compressor->GetUncompressedLength (compressed_str, &uncompressed_length)) {
    return uncompressed_length;
  } else {
    return 0;
  }
}

static SquashStatus
squash_gipfeli_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_length,
                                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                  SquashOptions* options) {
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();
  util::compression::UncheckedByteArraySink sink((char*) decompressed);
  util::compression::ByteArraySource source((const char*) compressed, compressed_length);

  std::string compressed_str((const char*) compressed, compressed_length);
  if (!compressor->GetUncompressedLength (compressed_str, decompressed_length))
    return squash_error (SQUASH_FAILED);

  if (!compressor->UncompressStream (&source, &sink)) {
    return squash_error (SQUASH_FAILED);
  }

  return SQUASH_OK;
}

static SquashStatus
squash_gipfeli_compress_buffer (SquashCodec* codec,
                                size_t* compressed_length,
                                uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                size_t uncompressed_length,
                                const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                SquashOptions* options) {
  util::compression::Compressor* compressor = util::compression::NewGipfeliCompressor();
  util::compression::UncheckedByteArraySink sink((char*) compressed);
  util::compression::ByteArraySource source((const char*) uncompressed, uncompressed_length);

  try {
    *compressed_length = compressor->CompressStream (&source, &sink);
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }

  return *compressed_length > 0 ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("gipfeli", name) == 0) {
    impl->get_uncompressed_size = squash_gipfeli_get_uncompressed_size;
    impl->get_max_compressed_size = squash_gipfeli_get_max_compressed_size;
    impl->decompress_buffer = squash_gipfeli_decompress_buffer;
    impl->compress_buffer_unsafe = squash_gipfeli_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
