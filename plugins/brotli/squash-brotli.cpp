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
 *   Evan Nemerson <evan@nemerson.com>
 */

#include <assert.h>

#include <squash/squash.h>

#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"

typedef struct SquashBrotliOptions_s {
  SquashOptions base_object;

  brotli::BrotliParams::Mode mode;
  bool enable_transforms;
} SquashBrotliOptions;

extern "C" SquashStatus     squash_plugin_init_codec      (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                 squash_brotli_options_init    (SquashBrotliOptions* options,
                                                           SquashCodec* codec,
                                                           SquashDestroyNotify destroy_notify);
static SquashBrotliOptions* squash_brotli_options_new     (SquashCodec* codec);
static void                 squash_brotli_options_destroy (void* options);
static void                 squash_brotli_options_free    (void* options);

static void
squash_brotli_options_init (SquashBrotliOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->mode = brotli::BrotliParams::MODE_TEXT;
  options->enable_transforms = false;
}

static SquashBrotliOptions*
squash_brotli_options_new (SquashCodec* codec) {
  SquashBrotliOptions* options;

  options = (SquashBrotliOptions*) malloc (sizeof (SquashBrotliOptions));
  squash_brotli_options_init (options, codec, squash_brotli_options_free);

  return options;
}

static void
squash_brotli_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_brotli_options_free (void* options) {
  squash_brotli_options_destroy ((SquashBrotliOptions*) options);
  free (options);
}

static SquashOptions*
squash_brotli_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_brotli_options_new (codec);
}

static SquashStatus
squash_brotli_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashBrotliOptions* opts = (SquashBrotliOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "mode") == 0) {
    if (strcasecmp (key, "text") == 0) {
      opts->mode = brotli::BrotliParams::MODE_TEXT;
    } else if (strcasecmp (key, "font")) {
      opts->mode = brotli::BrotliParams::MODE_FONT;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "enable-transforms") == 0) {
    if (strcasecmp (value, "true") == 0) {
      opts->enable_transforms = true;
    } else if (strcasecmp (value, "false")) {
      opts->enable_transforms = false;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static size_t
squash_brotli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + 4;
}

static SquashStatus
squash_brotli_decompress_buffer (SquashCodec* codec,
                                 uint8_t* decompressed, size_t* decompressed_length,
                                 const uint8_t* compressed, size_t compressed_length,
                                 SquashOptions* options) {
  int res = BrotliDecompressBuffer (compressed_length, compressed,
                                    decompressed_length, decompressed);

  return (res == 1) ? SQUASH_OK : SQUASH_FAILED;
}

static SquashStatus
squash_brotli_compress_buffer (SquashCodec* codec,
                            uint8_t* compressed, size_t* compressed_length,
                            const uint8_t* uncompressed, size_t uncompressed_length,
                            SquashOptions* options) {
  brotli::BrotliParams params;

  if (options != NULL) {
    SquashBrotliOptions* opts = (SquashBrotliOptions*) options;
    params.mode = opts->mode;
    params.enable_transforms = opts->enable_transforms;
  }

  int res = brotli::BrotliCompressBuffer (params,
                                          uncompressed_length, uncompressed,
                                          compressed_length, compressed);

  return (res == 1) ? SQUASH_OK : SQUASH_FAILED;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("brotli", name) == 0) {
    funcs->get_max_compressed_size = squash_brotli_get_max_compressed_size;
    funcs->decompress_buffer = squash_brotli_decompress_buffer;
    funcs->compress_buffer = squash_brotli_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
