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
#include <errno.h>

#include <squash/squash.h>

#include "fastlz/fastlz.h"

typedef struct SquashFastLZOptions_s {
  SquashOptions base_object;

  int level;
} SquashFastLZOptions;

SquashStatus                squash_plugin_init_codec      (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                 squash_fastlz_options_init    (SquashFastLZOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashFastLZOptions* squash_fastlz_options_new     (SquashCodec* codec);
static void                 squash_fastlz_options_destroy (void* options);
static void                 squash_fastlz_options_free    (void* options);

static void
squash_fastlz_options_init (SquashFastLZOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->level = 1;
}

static SquashFastLZOptions*
squash_fastlz_options_new (SquashCodec* codec) {
  SquashFastLZOptions* options;

  options = (SquashFastLZOptions*) malloc (sizeof (SquashFastLZOptions));
  squash_fastlz_options_init (options, codec, squash_fastlz_options_free);

  return options;
}

static void
squash_fastlz_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_fastlz_options_free (void* options) {
  squash_fastlz_options_destroy ((SquashFastLZOptions*) options);
  free (options);
}

static SquashOptions*
squash_fastlz_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_fastlz_options_new (codec);
}

static SquashStatus
squash_fastlz_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashFastLZOptions* opts = (SquashFastLZOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = (int) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 2 ) {
      opts->level = level;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else {
    return squash_error (SQUASH_BAD_PARAM);
  }

  return SQUASH_OK;
}

static size_t
squash_fastlz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  size_t max_compressed_size = uncompressed_length +
    (uncompressed_length / 20) + ((uncompressed_length % 20 == 0) ? uncompressed_length / 20 : 0);

  return (max_compressed_size < 66) ? 66 : max_compressed_size;
}

static SquashStatus
squash_fastlz_decompress_buffer (SquashCodec* codec,
                                 uint8_t* decompressed, size_t* decompressed_length,
                                 const uint8_t* compressed, size_t compressed_length,
                                 SquashOptions* options) {
  int fastlz_e = fastlz_decompress ((const void*) compressed,
                                    (int) compressed_length,
                                    (void*) decompressed,
                                    (int) *decompressed_length);

  if (fastlz_e < 0) {
    return squash_error (SQUASH_FAILED);
  } else {
    *decompressed_length = (size_t) fastlz_e;
    return SQUASH_OK;
  }
}

static SquashStatus
squash_fastlz_compress_buffer (SquashCodec* codec,
                               uint8_t* compressed, size_t* compressed_length,
                               const uint8_t* uncompressed, size_t uncompressed_length,
                               SquashOptions* options) {
  if (*compressed_length < squash_fastlz_get_max_compressed_size (codec, uncompressed_length)) {
    return SQUASH_BUFFER_FULL;
  }

  *compressed_length = fastlz_compress_level ((options == NULL) ? 1 : ((SquashFastLZOptions*) options)->level,
                                              (const void*) uncompressed,
                                              (int) uncompressed_length,
                                              (void*) compressed);

  return (*compressed_length == 0) ? squash_error (SQUASH_FAILED) : SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("fastlz", name) == 0) {
    funcs->create_options = squash_fastlz_create_options;
    funcs->parse_option = squash_fastlz_parse_option;
    funcs->get_max_compressed_size = squash_fastlz_get_max_compressed_size;
    funcs->decompress_buffer = squash_fastlz_decompress_buffer;
    funcs->compress_buffer = squash_fastlz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
