/* Copyright (c) 2013-2015 The Squash Authors
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

#include <squash/squash.h>

#include "wflz/wfLZ.h"

enum {
  SQUASH_WFLZ_LITTLE_ENDIAN = 0x03020100ul,
  SQUASH_WFLZ_BIG_ENDIAN = 0x00010203ul
};

static const union {
  unsigned char bytes[4];
  uint32_t value;
} squash_wflz_host_order = { { 0, 1, 2, 3 } };

#define SQUASH_WFLZ_HOST_ORDER (squash_wflz_host_order.value)

enum SquashWflzOptIndex {
  SQUASH_WFLZ_OPT_LEVEL = 0,
  SQUASH_WFLZ_OPT_CHUNK_SIZE,
  SQUASH_WFLZ_OPT_ENDIANNESS
};

static SquashOptionInfo squash_wflz_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 2 },
    .default_value.int_value = 1 },
  { "chunk-size",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 4096,
      .max = UINT32_MAX,
      .modulus = 16,
      .allow_zero = false },
    .default_value.int_value = 16384 },
  { "endianness",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "little", SQUASH_WFLZ_LITTLE_ENDIAN },
        { "big", SQUASH_WFLZ_BIG_ENDIAN },
        { NULL, 0 } } },
    .default_value.int_value = SQUASH_WFLZ_LITTLE_ENDIAN },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

#define SQUASH_WFLZ_DEFAULT_LEVEL 1
#define SQUASH_WFLZ_DEFAULT_ENDIAN SQUASH_WFLZ_LITTLE_ENDIAN
#define SQUASH_WFLZ_MIN_CHUNK_SIZE (1024 * 4)
#define SQUASH_WFLZ_DEFAULT_CHUNK_SIZE (1024 * 32)

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_wflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  const char* codec_name = squash_codec_get_name (codec);

#if UINT32_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(UINT32_MAX < uncompressed_size))
    return (squash_error (SQUASH_RANGE), 0);
#endif

  const uint32_t res = codec_name[4] == '\0' ?
    wfLZ_GetMaxCompressedSize ((uint32_t) uncompressed_size) :
    wfLZ_GetMaxChunkCompressedSize ((uint32_t) uncompressed_size, SQUASH_WFLZ_MIN_CHUNK_SIZE);

#if SIZE_MAX < UINT32_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < res))
    return (squash_error (SQUASH_RANGE));
#endif

  return res;
}

static size_t
squash_wflz_get_uncompressed_size (SquashCodec* codec,
                                   size_t compressed_size,
                                   const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)]) {
  if (compressed_size < 12)
    return 0;

  const uint32_t res = wfLZ_GetDecompressedSize (compressed);

#if SIZE_MAX < UINT32_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < res))
    return (squash_error (SQUASH_RANGE), 0);
#endif

  return (size_t) res;
}

static SquashStatus
squash_wflz_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  const char* codec_name = squash_codec_get_name (codec);
  const uint32_t swap = ((uint32_t) squash_codec_get_option_int_index (codec, options, SQUASH_WFLZ_OPT_ENDIANNESS) != SQUASH_WFLZ_HOST_ORDER);
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_WFLZ_OPT_LEVEL);

#if UINT32_MAX < SIZE_MAX
  if (SQUASH_UNLIKELY(UINT32_MAX < uncompressed_size))
    return squash_error (SQUASH_RANGE);
#endif

  if (*compressed_size < wfLZ_GetMaxCompressedSize ((uint32_t) uncompressed_size)) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  uint8_t* work_mem = (uint8_t*) malloc (wfLZ_GetWorkMemSize ());
  uint32_t wres;

  if (codec_name[4] == '\0') {
    if (level == 1) {
      wres = wfLZ_CompressFast (uncompressed, (uint32_t) uncompressed_size,
                                compressed, work_mem, swap);
    } else {
      wres = wfLZ_Compress (uncompressed, (uint32_t) uncompressed_size,
                            compressed, work_mem, swap);
    }
  } else {
    wres =
      wfLZ_ChunkCompress ((uint8_t*) uncompressed, (uint32_t) uncompressed_size,
                          squash_codec_get_option_size_index (codec, options, SQUASH_WFLZ_OPT_CHUNK_SIZE),
                          compressed, work_mem, swap, level == 1 ? 1 : 0);
  }

#if SIZE_MAX < UINT32_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < wres)) {
    free (work_mem);
    return squash_error (SQUASH_RANGE);
  }
#endif

  *compressed_size = (size_t) wres;

  free (work_mem);

  return SQUASH_LIKELY(*compressed_size > 0) ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

static SquashStatus
squash_wflz_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  const char* codec_name = squash_codec_get_name (codec);
  uint32_t decompressed_s;

  if (SQUASH_UNLIKELY(compressed_size < 12))
    return squash_error (SQUASH_BUFFER_EMPTY);

  decompressed_s = wfLZ_GetDecompressedSize (compressed);

#if SIZE_MAX < UINT32_MAX
  if (SQUASH_UNLIKELY(SIZE_MAX < decompressed_s))
    return squash_error (SQUASH_RANGE);
#endif

  if (SQUASH_UNLIKELY(decompressed_s == 0))
    return squash_error (SQUASH_INVALID_BUFFER);

  if (SQUASH_UNLIKELY(decompressed_s > *decompressed_size))
    return squash_error (SQUASH_BUFFER_FULL);

  if (codec_name[4] == '\0') {
    wfLZ_Decompress (compressed, decompressed);
  } else {
    uint8_t* dest = decompressed;
    uint32_t* chunk = NULL;
    uint8_t* compressed_block;

    while ( (compressed_block = wfLZ_ChunkDecompressLoop ((uint8_t*) compressed, &chunk)) != NULL ) {
      const uint32_t chunk_size = wfLZ_GetDecompressedSize (compressed_block);

      if (SQUASH_UNLIKELY((dest + chunk_size) > (decompressed + *decompressed_size)))
        return squash_error (SQUASH_BUFFER_FULL);

      wfLZ_Decompress (compressed_block, dest);
      dest += chunk_size;
    }
  }

  *decompressed_size = decompressed_s;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("wflz", name) == 0 ||
                    strcmp ("wflz-chunked", name) == 0)) {
    impl->options = squash_wflz_options;
    impl->get_uncompressed_size = squash_wflz_get_uncompressed_size;
    impl->get_max_compressed_size = squash_wflz_get_max_compressed_size;
    impl->decompress_buffer = squash_wflz_decompress_buffer;
    impl->compress_buffer_unsafe = squash_wflz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
