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
 *   Jeff Muizelaar <jrmuizel@gmail.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <squash/squash.h>

#include "libdeflate/libdeflate.h"

#define SQUASH_LIBDEFLATE_DEFAULT_LEVEL 3

enum SquashLibdeflateOptIndex {
  SQUASH_LIBDEFLATE_OPT_LEVEL = 0,
};

static SquashOptionInfo squash_libdeflate_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 12 },
    .default_value.int_value = 6 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec,
                                                         SquashCodecImpl* impl);

static size_t
squash_libdeflate_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return deflate_compress_bound(NULL, uncompressed_size);
}

static SquashStatus
squash_libdeflate_compress_buffer (SquashCodec* codec,
                              size_t* compressed_size,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_LIBDEFLATE_OPT_LEVEL);
  struct deflate_compressor *compressor = deflate_alloc_compressor(level);
  *compressed_size = deflate_compress(compressor, uncompressed, uncompressed_size, compressed, *compressed_size);
  deflate_free_compressor(compressor);
  return (*compressed_size != 0) ? SQUASH_OK : SQUASH_FAILED;
}

static SquashStatus
squash_libdeflate_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                SquashOptions* options) {
  struct deflate_decompressor *decompressor = deflate_alloc_decompressor();
  size_t actual_out_nbytes;
  enum decompress_result ret = deflate_decompress(decompressor, compressed, compressed_size,
                                             decompressed, *decompressed_size, &actual_out_nbytes);
  deflate_free_decompressor(decompressor);
  *decompressed_size = actual_out_nbytes;
  return ret == DECOMPRESS_SUCCESS ? SQUASH_OK : SQUASH_FAILED;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("deflate", name) == 0)) {
    impl->options = squash_libdeflate_options;
    impl->get_max_compressed_size = squash_libdeflate_get_max_compressed_size;
    impl->decompress_buffer = squash_libdeflate_decompress_buffer;
    impl->compress_buffer = squash_libdeflate_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
