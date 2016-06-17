/* Copyright (c) 2015-2016 The Squash Authors
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

#include "zstd.h"
#include "error_public.h"

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

enum SquashZstdOptIndex {
  SQUASH_ZSTD_OPT_LEVEL = 0
};

static SquashOptionInfo squash_zstd_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 22 },
    .default_value.int_value = 9 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static size_t
squash_zstd_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return ZSTD_compressBound (uncompressed_size);
}

static SquashStatus
squash_zstd_status_from_zstd_error (size_t res) {
  if (!ZSTD_isError (res))
    return SQUASH_OK;

  switch ((ZSTD_ErrorCode) (-(int)(res))) {
    case ZSTD_error_no_error:
      return SQUASH_OK;
    case ZSTD_error_memory_allocation:
      return squash_error (SQUASH_MEMORY);
    case ZSTD_error_dstSize_tooSmall:
      return squash_error (SQUASH_BUFFER_FULL);
    case ZSTD_error_GENERIC:
    case ZSTD_error_prefix_unknown:
    case ZSTD_error_frameParameter_unsupported:
    case ZSTD_error_frameParameter_unsupportedBy32bits:
    case ZSTD_error_init_missing:
    case ZSTD_error_stage_wrong:
    case ZSTD_error_srcSize_wrong:
    case ZSTD_error_corruption_detected:
    case ZSTD_error_tableLog_tooLarge:
    case ZSTD_error_maxSymbolValue_tooLarge:
    case ZSTD_error_maxSymbolValue_tooSmall:
    case ZSTD_error_dictionary_corrupted:
    case ZSTD_error_maxCode:
    case ZSTD_error_compressionParameter_unsupported:
    case ZSTD_error_checksum_wrong:
    case ZSTD_error_dictionary_wrong:
    default:
      return squash_error (SQUASH_FAILED);
  }

  squash_assert_unreachable ();
}

static SquashStatus
squash_zstd_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  *decompressed_size = ZSTD_decompress (decompressed, *decompressed_size, compressed, compressed_size);

  return squash_zstd_status_from_zstd_error (*decompressed_size);
}

static SquashStatus
squash_zstd_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  const int level = squash_options_get_int_at (options, codec, SQUASH_ZSTD_OPT_LEVEL);

  *compressed_size = ZSTD_compress (compressed, *compressed_size, uncompressed, uncompressed_size, level);

  return squash_zstd_status_from_zstd_error (*compressed_size);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("zstd", name) == 0)) {
    impl->options = squash_zstd_options;
    impl->get_max_compressed_size = squash_zstd_get_max_compressed_size;
    impl->decompress_buffer = squash_zstd_decompress_buffer;
    impl->compress_buffer_unsafe = squash_zstd_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
