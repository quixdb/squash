/* Copyright (c) 2013-2016 The Squash Authors
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
 *   Ivan Tkatchev <tkatchev@gmail.com>
 *   Evan Nemerson <evan@nemerson.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <squash/squash.h>

#include "lz77.h"

enum SquashYalz77OptIndex {
  SQUASH_YALZ77_OPT_SEARCH_LENGTH = 0,
  SQUASH_YALZ77_OPT_BLOCK_SIZE
};

static SquashOptionInfo squash_yalz77_options[] = {
  { (char*) "search-length",
    SQUASH_OPTION_TYPE_SIZE, },
  { (char*) "block-size",
    SQUASH_OPTION_TYPE_SIZE },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec  (SquashCodec* codec, SquashCodecImpl* impl);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_plugin (SquashPlugin* plugin);

static size_t
squash_yalz77_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 16 + (uncompressed_size / (1024 * 512));
}

static SquashStatus
squash_yalz77_compress_buffer (SquashCodec* codec,
                               size_t* compressed_size,
                               uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                               size_t uncompressed_size,
                               const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                               SquashOptions* options) {

  const size_t searchlen = squash_codec_get_option_size_index (codec, options, SQUASH_YALZ77_OPT_SEARCH_LENGTH);
  const size_t blocksize = squash_codec_get_option_size_index (codec, options, SQUASH_YALZ77_OPT_BLOCK_SIZE);

  try {
    lz77::compress_t compress(searchlen, blocksize);
    std::string res = compress.feed(uncompressed, uncompressed + uncompressed_size);

    if (SQUASH_UNLIKELY(res.size() > *compressed_size))
      return squash_error (SQUASH_BUFFER_FULL);

    memcpy(compressed, res.c_str(), res.size());
    *compressed_size = res.size();
    return SQUASH_OK;
  } catch (const std::bad_alloc& e) {
    (void) e;
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }
}

static SquashStatus
squash_yalz77_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_size,
                                 uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                 size_t compressed_size,
                                 const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                 SquashOptions* options) {
  try {
    lz77::decompress_t decompress(*decompressed_size);
    std::string remaining;
    bool done = decompress.feed(compressed, compressed + compressed_size, remaining);
    const std::string& res = decompress.result();

    memcpy(decompressed, res.c_str(), res.size());
    *decompressed_size = res.size();
    return (done && remaining.empty()) ? SQUASH_OK : SQUASH_FAILED;
  } catch (std::length_error& e) {
    (void) e;
    return squash_error (SQUASH_BUFFER_FULL);
  } catch (const std::bad_alloc& e) {
    (void) e;
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }
}

extern "C" SquashStatus
squash_plugin_init_plugin (SquashPlugin* plugin) {
  squash_yalz77_options[SQUASH_YALZ77_OPT_SEARCH_LENGTH].default_value.size_value = 8;
  squash_yalz77_options[SQUASH_YALZ77_OPT_BLOCK_SIZE].default_value.size_value = 65536;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("yalz77", name) == 0)) {
    impl->options = squash_yalz77_options;
    impl->get_max_compressed_size = squash_yalz77_get_max_compressed_size;
    impl->decompress_buffer = squash_yalz77_decompress_buffer;
    impl->compress_buffer = squash_yalz77_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
