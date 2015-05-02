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

#include "liblzg/src/include/lzg.h"

typedef struct SquashLzgOptions_s {
  SquashOptions base_object;

  lzg_encoder_config_t lzgcfg;
} SquashLzgOptions;

const lzg_encoder_config_t squash_lzg_default_config = {
  LZG_LEVEL_DEFAULT,
  LZG_TRUE,
  NULL,
  NULL
};

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_lzg_options_init    (SquashLzgOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashLzgOptions* squash_lzg_options_new     (SquashCodec* codec);
static void              squash_lzg_options_destroy (void* options);
static void              squash_lzg_options_free    (void* options);

static void
squash_lzg_options_init (SquashLzgOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->lzgcfg = squash_lzg_default_config;
}

static SquashLzgOptions*
squash_lzg_options_new (SquashCodec* codec) {
  SquashLzgOptions* options;

  options = (SquashLzgOptions*) malloc (sizeof (SquashLzgOptions));
  squash_lzg_options_init (options, codec, squash_lzg_options_free);

  return options;
}

static void
squash_lzg_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_lzg_options_free (void* options) {
  squash_lzg_options_destroy ((SquashLzgOptions*) options);
  free (options);
}

static SquashOptions*
squash_lzg_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_lzg_options_new (codec);
}

static SquashStatus
squash_lzg_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashLzgOptions* opts = (SquashLzgOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const lzg_int32_t level = (lzg_int32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 9 ) {
      opts->lzgcfg.level = level;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "fast") == 0) {
    if (strcasecmp (value, "true") == 0) {
      opts->lzgcfg.fast = LZG_TRUE;
    } else if (strcasecmp (value, "false")) {
      opts->lzgcfg.fast = LZG_FALSE;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else {
    return squash_error (SQUASH_BAD_PARAM);
  }

  return SQUASH_OK;
}

static size_t
squash_lzg_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return (size_t) LZG_MaxEncodedSize ((lzg_uint32_t) uncompressed_length);
}

static size_t
squash_lzg_get_uncompressed_size (SquashCodec* codec,
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  return (size_t) LZG_DecodedSize ((const unsigned char*) compressed, (lzg_uint32_t) compressed_length);
}

static SquashStatus
squash_lzg_compress_buffer (SquashCodec* codec,
                            size_t* compressed_length,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOptions* options) {
  lzg_encoder_config_t* cfg;

  if (options != NULL) {
    cfg = &(((SquashLzgOptions*) options)->lzgcfg);
  } else {
    cfg = (lzg_encoder_config_t*) (&squash_lzg_default_config);
  }

  lzg_uint32_t res = LZG_Encode ((const unsigned char*) uncompressed, (lzg_uint32_t) uncompressed_length,
                                 (unsigned char*) compressed, (lzg_uint32_t) *compressed_length,
                                 cfg);

  if (res == 0) {
    return squash_error (SQUASH_FAILED);
  } else {
    *compressed_length = (size_t) res;
    return SQUASH_OK;
  }
}

static SquashStatus
squash_lzg_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_length,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                              size_t compressed_length,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                              SquashOptions* options) {
  lzg_uint32_t res = LZG_Decode ((const unsigned char*) compressed, (lzg_uint32_t) compressed_length,
                                 (unsigned char*) decompressed, (lzg_uint32_t) *decompressed_length);

  if (res == 0) {
    return squash_error (SQUASH_FAILED);
  } else {
    *decompressed_length = (size_t) res;
    return SQUASH_OK;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzg", name) == 0) {
    funcs->create_options = squash_lzg_create_options;
    funcs->parse_option = squash_lzg_parse_option;
    funcs->get_uncompressed_size = squash_lzg_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_lzg_get_max_compressed_size;
    funcs->decompress_buffer = squash_lzg_decompress_buffer;
    funcs->compress_buffer = squash_lzg_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
