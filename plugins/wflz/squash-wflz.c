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
#include <strings.h>

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
squash_wflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  const char* codec_name = squash_codec_get_name (codec);
  return codec_name[4] == '\0' ?
    (size_t) wfLZ_GetMaxCompressedSize ((uint32_t) uncompressed_length) :
    (size_t) wfLZ_GetMaxChunkCompressedSize ((uint32_t) uncompressed_length, SQUASH_WFLZ_MIN_CHUNK_SIZE);
}

static size_t
squash_wflz_get_uncompressed_size (SquashCodec* codec,
                                   size_t compressed_length,
                                   const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  if (compressed_length < 12)
    return 0;

  return (size_t) wfLZ_GetDecompressedSize (compressed);
}

static SquashStatus
squash_wflz_compress_buffer (SquashCodec* codec,
                             size_t* compressed_length,
                             uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                             size_t uncompressed_length,
                             const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                             SquashOptions* options) {
  const char* codec_name = squash_codec_get_name (codec);
  const uint32_t swap = (squash_codec_get_option_int_index (codec, options, SQUASH_WFLZ_OPT_ENDIANNESS) != SQUASH_WFLZ_HOST_ORDER);
  const int level = squash_codec_get_option_int_index (codec, options, SQUASH_WFLZ_OPT_LEVEL);

  if (*compressed_length < wfLZ_GetMaxCompressedSize ((uint32_t) uncompressed_length)) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  uint8_t* work_mem = (uint8_t*) malloc (wfLZ_GetWorkMemSize ());

  if (codec_name[4] == '\0') {
    if (level == 1) {
      *compressed_length = (size_t) wfLZ_CompressFast (uncompressed, (uint32_t) uncompressed_length,
                                                       compressed, work_mem, swap);
    } else {
      *compressed_length = (size_t) wfLZ_Compress (uncompressed, (uint32_t) uncompressed_length,
                                                   compressed, work_mem, swap);
    }
  } else {
    *compressed_length = (size_t)
      wfLZ_ChunkCompress ((uint8_t*) uncompressed, (uint32_t) uncompressed_length,
                          squash_codec_get_option_size_index (codec, options, SQUASH_WFLZ_OPT_CHUNK_SIZE),
                          compressed, work_mem, swap, level == 1 ? 1 : 0);
  }

  free (work_mem);

  return (*compressed_length > 0) ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

static SquashStatus
squash_wflz_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_length,
                               uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                               size_t compressed_length,
                               const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                               SquashOptions* options) {
  const char* codec_name = squash_codec_get_name (codec);
  uint32_t decompressed_size;

  if (compressed_length < 12 || (decompressed_size = wfLZ_GetDecompressedSize (compressed)) > *decompressed_length) {
    return squash_error (SQUASH_FAILED);
  }

  if (codec_name[4] == '\0') {
    wfLZ_Decompress (compressed, decompressed);
  } else {
    uint8_t* dest = decompressed;
    uint32_t* chunk = NULL;
    uint8_t* compressed_block;

    while ( (compressed_block = wfLZ_ChunkDecompressLoop ((uint8_t*) compressed, &chunk)) != NULL ) {
      const uint32_t chunk_length = wfLZ_GetDecompressedSize (compressed_block);

      if ((dest + chunk_length) > (decompressed + *decompressed_length))
        return squash_error (SQUASH_BUFFER_FULL);

      wfLZ_Decompress (compressed_block, dest);
      dest += chunk_length;
    }
  }

  *decompressed_length = decompressed_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("wflz", name) == 0 ||
      strcmp ("wflz-chunked", name) == 0) {
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
