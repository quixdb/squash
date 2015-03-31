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

#include "lz77.h"

typedef struct SquashYalz77Options_s {
  SquashOptions base_object;

  size_t searchlen;
  size_t blocksize;
} SquashYalz77Options;

static void
squash_yalz77_options_init (SquashYalz77Options* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->searchlen = lz77::DEFAULT_SEARCHLEN;
  options->blocksize = lz77::DEFAULT_BLOCKSIZE;
}

static void
squash_yalz77_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_yalz77_options_free (void* options) {
  squash_yalz77_options_destroy ((SquashYalz77Options*) options);
  free (options);
}

static SquashYalz77Options*
squash_yalz77_options_new (SquashCodec* codec) {
  SquashYalz77Options* options;

  options = (SquashYalz77Options*) malloc (sizeof (SquashYalz77Options));
  squash_yalz77_options_init (options, codec, squash_yalz77_options_free);

  return options;
}

static SquashOptions*
squash_yalz77_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_yalz77_options_new (codec);
}

static SquashStatus
squash_yalz77_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashYalz77Options* opts = (SquashYalz77Options*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "searchlen") == 0) {
    const size_t searchlen = (size_t) strtoul (value, &endptr, 0);
    if ( *endptr == '\0' && searchlen > 0) {
      opts->searchlen = searchlen;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "blocksize") == 0) {
    const size_t blocksize = (size_t) strtoul (value, &endptr, 0);
    if ( *endptr == '\0' && blocksize > 0) {
      opts->blocksize = blocksize;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static size_t
squash_yalz77_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + 4;
}

static SquashStatus
squash_yalz77_compress_buffer (SquashCodec* codec,
                               uint8_t* compressed, size_t* compressed_length,
                               const uint8_t* uncompressed, size_t uncompressed_length,
                               SquashOptions* options) {

  size_t searchlen = lz77::DEFAULT_SEARCHLEN;
  size_t blocksize = lz77::DEFAULT_BLOCKSIZE;
  
  if (options != NULL) {
    SquashYalz77Options* opts = (SquashYalz77Options*) options;
    searchlen = opts->searchlen;
    blocksize = opts->blocksize;
  }

  try {
    lz77::compress_t compress(searchlen, blocksize);
    std::string res = compress.feed(uncompressed, uncompressed + uncompressed_length);

    if (res.size() > *compressed_length)
      return SQUASH_FAILED;

    memcpy(compressed, res.c_str(), res.size());
    *compressed_length = res.size();
    return SQUASH_OK;
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }
}

static SquashStatus
squash_yalz77_decompress_buffer (SquashCodec* codec,
                                 uint8_t* decompressed, size_t* decompressed_length,
                                 const uint8_t* compressed, size_t compressed_length,
                                 SquashOptions* options) {
  try {
    lz77::decompress_t decompress;
    std::string remaining;
    bool done = decompress.feed(compressed, compressed + compressed_length, remaining);
    const std::string& res = decompress.result();

    if (res.size() > *decompressed_length)
      return SQUASH_FAILED;

    memcpy(decompressed, res.c_str(), res.size());
    *decompressed_length = res.size();
    return (done && remaining.empty()) ? SQUASH_OK : SQUASH_FAILED;
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("yalz77", name) == 0) {
    funcs->create_options = squash_yalz77_create_options;
    funcs->parse_option = squash_yalz77_parse_option;
    funcs->get_max_compressed_size = squash_yalz77_get_max_compressed_size;
    funcs->decompress_buffer = squash_yalz77_decompress_buffer;
    funcs->compress_buffer = squash_yalz77_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
