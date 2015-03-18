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
#include <limits.h>

#include <squash/squash.h>

#include "libbsc/libbsc/libbsc.h"

typedef struct SquashBscOptions_s {
  SquashOptions base_object;
  int lzp_hash_size;
  int lzp_min_len;
  int block_sorter;
  int coder;
  int feature;
} SquashBscOptions;


SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_bsc_options_init    (SquashBscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashBscOptions* squash_bsc_options_new     (SquashCodec* codec);
static void              squash_bsc_options_destroy (void* options);
static void              squash_bsc_options_free    (void* options);

static void
squash_bsc_options_init (SquashBscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->lzp_hash_size = LIBBSC_DEFAULT_LZPHASHSIZE;
  options->lzp_min_len = LIBBSC_DEFAULT_LZPMINLEN;
  options->block_sorter = LIBBSC_DEFAULT_BLOCKSORTER;
  options->coder = LIBBSC_DEFAULT_CODER;
  options->feature = LIBBSC_DEFAULT_FEATURES;
}

static SquashBscOptions*
squash_bsc_options_new (SquashCodec* codec) {
  SquashBscOptions* options;

  options = (SquashBscOptions*) malloc (sizeof (SquashBscOptions));
  squash_bsc_options_init (options, codec, squash_bsc_options_free);

  return options;
}

static void
squash_bsc_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_bsc_options_free (void* options) {
  squash_bsc_options_destroy ((SquashBscOptions*) options);
  free (options);
}

static SquashOptions*
squash_bsc_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_bsc_options_new (codec);
}

static bool
string_to_bool (const char* value, bool* result) {
  if (strcasecmp (value, "true") == 0) {
    *result = true;
  } else if (strcasecmp (value, "false")) {
    *result = false;
  } else {
    return false;
  }
  return true;
}

static SquashStatus
squash_bsc_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashBscOptions* opts = (SquashBscOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "lzp-hash-size") == 0) {
    const int lzp_hash_size = (int) strtol (value, &endptr, 0);
    if (*endptr == '\0' && (lzp_hash_size == 0 || (lzp_hash_size >= 10 && lzp_hash_size <= 28))) {
      opts->lzp_hash_size = lzp_hash_size;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "lzp-min-len") == 0) {
    const int lzp_min_len = (int) strtol (value, &endptr, 0);
    if (*endptr == '\0' && (lzp_min_len == 0 || (lzp_min_len >= 4 && lzp_min_len <= 255))) {
      opts->lzp_min_len = lzp_min_len;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "block-sorter") == 0) {
    if (strcasecmp (value, "none") == 0) {
      opts->block_sorter = LIBBSC_BLOCKSORTER_NONE;
    } else if (strcasecmp (value, "bwt") == 0) {
      opts->block_sorter = LIBBSC_BLOCKSORTER_BWT;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "coder") == 0) {
    if (strcasecmp (value, "none") == 0) {
      opts->coder = LIBBSC_CODER_NONE;
    } else if (strcasecmp (value, "qflc-static") == 0) {
      opts->coder = LIBBSC_CODER_QLFC_STATIC;
    } else if (strcasecmp (value, "qflc-adaptive") == 0) {
      opts->coder = LIBBSC_CODER_QLFC_ADAPTIVE;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "fast-mode") == 0) {
    bool res;
    bool valid = string_to_bool (value, &res);
    if (valid)
      opts->feature = res ? (opts->feature | LIBBSC_FEATURE_FASTMODE) : (opts->feature & ~LIBBSC_FEATURE_FASTMODE);
    else
      return SQUASH_BAD_VALUE;
  } else if (strcasecmp (key, "multi-threading") == 0) {
    bool res;
    bool valid = string_to_bool (value, &res);
    if (valid)
      opts->feature = res ? (opts->feature | LIBBSC_FEATURE_MULTITHREADING) : (opts->feature & ~LIBBSC_FEATURE_MULTITHREADING);
    else
      return SQUASH_BAD_VALUE;
  } else if (strcasecmp (key, "large-pages") == 0) {
    bool res;
    bool valid = string_to_bool (value, &res);
    if (valid)
      opts->feature = res ? (opts->feature | LIBBSC_FEATURE_LARGEPAGES) : (opts->feature & ~LIBBSC_FEATURE_LARGEPAGES);
    else
      return SQUASH_BAD_VALUE;
  } else if (strcasecmp (key, "cuda") == 0) {
    bool res;
    bool valid = string_to_bool (value, &res);
    if (valid)
      opts->feature = res ? (opts->feature | LIBBSC_FEATURE_CUDA) : (opts->feature & ~LIBBSC_FEATURE_CUDA);
    else
      return SQUASH_BAD_VALUE;
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static size_t
squash_bsc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + LIBBSC_HEADER_SIZE;
}

static size_t
squash_lzg_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  int p_block_size, p_data_size;

  int res = bsc_block_info (compressed, (int) compressed_length, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (res != LIBBSC_NO_ERROR) {
    return 0;
  } else {
    return (size_t) p_data_size;
  }
}

static SquashStatus
squash_bsc_compress_buffer (SquashCodec* codec,
                            uint8_t* compressed, size_t* compressed_length,
                            const uint8_t* uncompressed, size_t uncompressed_length,
                            SquashOptions* options) {
  int lzp_hash_size = LIBBSC_DEFAULT_LZPHASHSIZE;
  int lzp_min_len = LIBBSC_DEFAULT_LZPMINLEN;
  int block_sorter = LIBBSC_DEFAULT_BLOCKSORTER;
  int coder = LIBBSC_DEFAULT_CODER;
  int feature = LIBBSC_DEFAULT_FEATURES;

  if (options != NULL) {
    SquashBscOptions* opts = (SquashBscOptions*) options;
    lzp_hash_size = opts->lzp_hash_size;
    lzp_min_len = opts->lzp_min_len;
    block_sorter = opts->block_sorter;
    coder = opts->coder;
    feature = opts->feature;
  }

  if (*compressed_length < (uncompressed_length + LIBBSC_HEADER_SIZE))
    return SQUASH_BUFFER_FULL;

  int res = bsc_compress (uncompressed, compressed, (int) uncompressed_length,
                          lzp_hash_size, lzp_min_len, block_sorter, coder, feature);

  if (res < 0) {
    return SQUASH_FAILED;
  }

  *compressed_length = (size_t) res;

  return SQUASH_OK;
}

static SquashStatus
squash_bsc_decompress_buffer (SquashCodec* codec,
                              uint8_t* decompressed, size_t* decompressed_length,
                              const uint8_t* compressed, size_t compressed_length,
                              SquashOptions* options) {
  assert (compressed_length < (size_t) INT_MAX);
  assert (*decompressed_length < (size_t) INT_MAX);

  int feature = LIBBSC_DEFAULT_FEATURES;
  if (options != NULL) {
    SquashBscOptions* opts = (SquashBscOptions*) options;
    feature = opts->feature;
  }

  int p_block_size, p_data_size;

  int res = bsc_block_info (compressed, (int) compressed_length, &p_block_size, &p_data_size, LIBBSC_DEFAULT_FEATURES);

  if (p_block_size != (int) compressed_length)
    return SQUASH_FAILED;
  if (p_data_size < (int) *decompressed_length)
    return SQUASH_BUFFER_FULL;

  res = bsc_decompress (compressed, p_block_size, decompressed, p_data_size, feature);

  if (res < 0)
    return SQUASH_FAILED;

  *decompressed_length = (size_t) p_data_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  bsc_init (LIBBSC_DEFAULT_FEATURES);

  const char* name = squash_codec_get_name (codec);

  if (strcmp ("bsc", name) == 0) {
    funcs->create_options = squash_bsc_create_options;
    funcs->parse_option = squash_bsc_parse_option;
    funcs->get_uncompressed_size = squash_lzg_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_bsc_get_max_compressed_size;
    funcs->decompress_buffer = squash_bsc_decompress_buffer;
    funcs->compress_buffer = squash_bsc_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
