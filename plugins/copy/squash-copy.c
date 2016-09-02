/* Copyright (c) 2015-2016 The Squash Authors
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

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec    (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_copy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size;
}

static size_t
squash_copy_get_uncompressed_size (SquashCodec* codec,
                                   size_t compressed_size,
                                   const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)]) {
  return compressed_size;
}

static bool
squash_copy_init_stream (SquashStream* stream,
                         SquashStreamType stream_type,
                         SquashOptions* options,
                         void* priv) {
  return true;
}

static void
squash_copy_destroy_stream (SquashStream* stream, void* priv) {
}

static SquashStatus
squash_copy_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  const size_t cp_size = stream->avail_in < stream->avail_out ? stream->avail_in : stream->avail_out;

  if (cp_size != 0) {
    memcpy (stream->next_out, stream->next_in, cp_size);
    stream->avail_in -= cp_size;
    stream->next_in += cp_size;
    stream->avail_out -= cp_size;
    stream->next_out += cp_size;
  }

  return (stream->avail_in != 0) ? SQUASH_PROCESSING : SQUASH_OK;
}

static SquashStatus
squash_copy_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  if (HEDLEY_UNLIKELY(*compressed_size < uncompressed_size))
    return squash_error (SQUASH_BUFFER_FULL);

  memcpy (compressed, uncompressed, uncompressed_size);
  *compressed_size = uncompressed_size;

  return SQUASH_OK;
}

static SquashStatus
squash_copy_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  if (HEDLEY_UNLIKELY(*decompressed_size < compressed_size))
    return squash_error (SQUASH_BUFFER_FULL);

  memcpy (decompressed, compressed, compressed_size);
  *decompressed_size = compressed_size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("copy", name) == 0)) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->get_uncompressed_size = squash_copy_get_uncompressed_size;
    impl->get_max_compressed_size = squash_copy_get_max_compressed_size;
    impl->decompress_buffer = squash_copy_decompress_buffer;
    impl->compress_buffer = squash_copy_compress_buffer;
    impl->init_stream = squash_copy_init_stream;
    impl->destroy_stream = squash_copy_destroy_stream;
    impl->process_stream = squash_copy_process_stream;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
