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

typedef struct SquashWflzOptions_s {
  SquashOptions base_object;

  int level;
  uint32_t endianness;
  uint32_t chunk_size;
} SquashWflzOptions;

#define SQUASH_WFLZ_DEFAULT_LEVEL 1
#define SQUASH_WFLZ_DEFAULT_ENDIAN SQUASH_WFLZ_LITTLE_ENDIAN
#define SQUASH_WFLZ_MIN_CHUNK_SIZE (1024 * 4)
#define SQUASH_WFLZ_DEFAULT_CHUNK_SIZE (1024 * 32)

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs);

static void               squash_wflz_options_init    (SquashWflzOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashWflzOptions* squash_wflz_options_new     (SquashCodec* codec);
static void               squash_wflz_options_destroy (void* options);
static void               squash_wflz_options_free    (void* options);

static void
squash_wflz_options_init (SquashWflzOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->level = SQUASH_WFLZ_DEFAULT_LEVEL;
  options->endianness = SQUASH_WFLZ_LITTLE_ENDIAN;
  options->chunk_size = SQUASH_WFLZ_DEFAULT_CHUNK_SIZE;
}

static SquashWflzOptions*
squash_wflz_options_new (SquashCodec* codec) {
  SquashWflzOptions* options;

  options = (SquashWflzOptions*) malloc (sizeof (SquashWflzOptions));
  squash_wflz_options_init (options, codec, squash_wflz_options_free);

  return options;
}

static void
squash_wflz_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_wflz_options_free (void* options) {
  squash_wflz_options_destroy ((SquashWflzOptions*) options);
  free (options);
}

static SquashOptions*
squash_wflz_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_wflz_options_new (codec);
}

static SquashStatus
squash_wflz_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashWflzOptions* opts = (SquashWflzOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const long level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 2 ) {
      opts->level = (int) level;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "endianness") == 0) {
    if (strcasecmp (value, "little") == 0) {
      opts->endianness = SQUASH_WFLZ_LITTLE_ENDIAN;
    } else if (strcasecmp (value, "big")) {
      opts->endianness = SQUASH_WFLZ_BIG_ENDIAN;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcmp (squash_codec_get_name (options->codec), "wflz-chunked") == 0 && strcasecmp (key, "chunk-size") == 0) {
    const long level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= SQUASH_WFLZ_MIN_CHUNK_SIZE && (level % 16) == 0 ) {
      opts->chunk_size = (uint32_t) level;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static size_t
squash_wflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  const char* codec_name = squash_codec_get_name (codec);
  return codec_name[4] == '\0' ?
    (size_t) wfLZ_GetMaxCompressedSize ((uint32_t) uncompressed_length) :
    (size_t) wfLZ_GetMaxChunkCompressedSize ((uint32_t) uncompressed_length, SQUASH_WFLZ_MIN_CHUNK_SIZE);
}

static size_t
squash_wflz_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  if (compressed_length < 12)
    return 0;

  return (size_t) wfLZ_GetDecompressedSize (compressed);
}

static SquashStatus
squash_wflz_compress_buffer (SquashCodec* codec,
                             uint8_t* compressed, size_t* compressed_length,
                             const uint8_t* uncompressed, size_t uncompressed_length,
                             SquashOptions* options) {
  const char* codec_name = squash_codec_get_name (codec);
  const uint32_t swap = ((options == NULL) ? SQUASH_WFLZ_DEFAULT_ENDIAN : ((SquashWflzOptions*) options)->endianness) != SQUASH_WFLZ_HOST_ORDER;
  const int level = (options == NULL) ? SQUASH_WFLZ_DEFAULT_LEVEL : ((SquashWflzOptions*) options)->level;

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
                          (options == NULL) ? SQUASH_WFLZ_DEFAULT_CHUNK_SIZE : ((SquashWflzOptions*) options)->chunk_size,
                          compressed, work_mem, swap, level == 1 ? 1 : 0);
  }

  free (work_mem);

  return (*compressed_length > 0) ? SQUASH_OK : squash_error (SQUASH_FAILED);
}

static SquashStatus
squash_wflz_decompress_buffer (SquashCodec* codec,
                               uint8_t* decompressed, size_t* decompressed_length,
                               const uint8_t* compressed, size_t compressed_length,
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
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("wflz", name) == 0 ||
      strcmp ("wflz-chunked", name) == 0) {
    funcs->create_options = squash_wflz_create_options;
    funcs->parse_option = squash_wflz_parse_option;
    funcs->get_uncompressed_size = squash_wflz_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_wflz_get_max_compressed_size;
    funcs->decompress_buffer = squash_wflz_decompress_buffer;
    funcs->compress_buffer_unsafe = squash_wflz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
