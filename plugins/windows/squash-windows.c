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
#include <Ntifs.h>

#include <squash/squash.h>

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_windows_lznt1_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 256;
}

static size_t
squash_windows_xpress_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 256;
}

static size_t
squash_windows_xpress_huffman_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 256;
}

enum SquashWindowsOptIndex {
  SQUASH_WINDOWS_OPT_LEVEL = 0
};

static SquashOptionInfo squash_windows_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 2 },
    .default_value.int_value = 1 },
};

static USHORT squash_windows_get_format_from_codec (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);

  if (name[0] == 'l')
    return COMPRESSION_FORMAT_LZNT1;
  else if (name[6] == '-')
    return COMPRESSION_FORMAT_XPRESS_HUFF;
  else
    return COMPRESSION_FORMAT_XPRESS;
}

static SquashStatus
squash_windows_compress_buffer (SquashCodec* codec,
                                size_t* compressed_size,
                                uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                                size_t uncompressed_size,
                                const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                                SquashOptions* options) {
  const USHORT engine = squash_options_get_int_at (options, codec, SQUASH_ZLIB_OPT_LEVEL) == 1 ?
    COMPRESSION_ENGINE_STANDARD : COMPRESSION_ENGINE_MAXIMUM;
  const USHORT format = squash_windows_get_format_from_codec (codec);
  const USHORT format_and_engine = format | engine;
  uint8_t* workmem = NULL;
  ULONG workmem_size;
  NTSTATUS r;
  ULONG out_size;

#if ULONG_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(ULONG_MAX < *compressed_size) ||
      SQUASH_UNLIKELY(ULONG_MAX < decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  r = RtlGetCompressionWorkSpaceSize (format_and_engine, &workmem_size, NULL);
  if (SQUASH_UNLIKELY(r != STATUS_SUCCESS))
    return squash_error (SQUASH_FAILED);

  workmem = squash_malloc(workmem_size);
  if (SQUASH_UNLIKELY(workmem == NULL))
    return squash_error (SQUASH_MEMORY);

  r = RtlCompressBuffer (format_and_engine,
                         (PUCHAR) uncompressed, (ULONG) uncompressed_size,
                         (PUCHAR) compressed, (ULONG) *compressed_size,
                         4096, &out_size, workmem);

  squash_free (workmem);

  if (SQUASH_UNLIKELY(r != STATUS_SUCCCESS)) {
    switch (r) {
      case STATUS_BUFFER_ALL_ZEROS:
        break;
      case STATUS_BUFFER_TOO_SMALL:
        return squash_error (SQUASH_BUFFER_FULL);
      default:
        return squash_error (SQUASH_FAILED);
    }
  }

#if SIZE_MAX < ULONG_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < out_size))
    return squash_error (SQUASH_RANGE);
#endif

  *compressed_size = (size_t) out_size;

  return SQUASH_OK;
}

static SquashStatus
squash_windows_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_size,
                                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                  size_t compressed_size,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                  SquashOptions* options) {
  const USHORT format = squash_windows_get_format_from_codec (codec);
  ULONG out_size;
  NTSTATUS r;

#if ULONG_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(ULONG_MAX < compressed_size) ||
      SQUASH_UNLIKELY(ULONG_MAX < *decompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  r = RtlDecompressBuffer(format,
                          (PUCHAR) decompressed, (ULONG) *decompressed_size,
                          (PUCHAR) compressed, (ULONG) compressed_size,
                          &out_size);

#if SIZE_MAX < ULONG_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < out_size))
    return squash_error (SQUASH_RANGE);
#endif

  if (SQUASH_UNLIKELY(r != STATUS_SUCCESS)) {
    switch (r) {
      case STATUS_BAD_COMPRESSION_BUFFER:
        return squash_error (SQUASH_BUFFER_FULL);
      default:
        return squash_error (SQUASH_FAILED);
    }
  }

  *decompressed_size = (size_t) out_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lznt1", name) == 0)
    impl->get_max_compressed_size = squash_windows_lznt1_get_max_compressed_size;
  else if (strcmp ("xpress", name) == 0)
    impl->get_max_compressed_size = squash_windows_xpress_get_max_compressed_size;
  else if (strcmp ("xpress-huffman", name) == 0)
    impl->get_max_compressed_size = squash_windows_xpress_huffman_get_max_compressed_size;
  else
    return squash_error (SQUASH_UNABLE_TO_LOAD);

  impl->options = squash_windows_options;
  impl->decompress_buffer = squash_windows_decompress_buffer;
  impl->compress_buffer = squash_windows_compress_buffer;

  return SQUASH_OK;
}
