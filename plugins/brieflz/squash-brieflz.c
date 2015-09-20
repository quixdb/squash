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
 *   Joergen Ibsen
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <squash/squash.h>
#include <brieflz.h>

#include <stdio.h>

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_brieflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return (size_t) blz_max_packed_size ((unsigned long) uncompressed_length) + 4;
}

static SquashStatus
squash_brieflz_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_length,
                                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                  SquashOptions* options) {
  unsigned long original_size;
  unsigned long size;

  original_size = (unsigned long) compressed[0]
               | ((unsigned long) compressed[1] << 8)
               | ((unsigned long) compressed[2] << 16)
               | ((unsigned long) compressed[3] << 24);

  if (original_size > *decompressed_length) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  size = blz_depack (compressed + 4, decompressed, original_size);

  if (size != original_size) {
    return squash_error (SQUASH_FAILED);
  }

  *decompressed_length = (size_t) size;

  return SQUASH_OK;
}

static SquashStatus
squash_brieflz_compress_buffer (SquashCodec* codec,
                                size_t* compressed_length,
                                uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                size_t uncompressed_length,
                                const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                SquashOptions* options) {
  unsigned long size;
  void *workmem = NULL;

  if ((unsigned long) *compressed_length
    < squash_brieflz_get_max_compressed_size (codec, uncompressed_length)) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  workmem = malloc (blz_workmem_size ((unsigned long) uncompressed_length));

  if (workmem == NULL) {
    return squash_error (SQUASH_MEMORY);
  }

  compressed[0] = (uint8_t) uncompressed_length;
  compressed[1] = (uint8_t) (uncompressed_length >> 8);
  compressed[2] = (uint8_t) (uncompressed_length >> 16);
  compressed[3] = (uint8_t) (uncompressed_length >> 24);

  size = blz_pack (uncompressed, compressed + 4,
                   (unsigned long) uncompressed_length,
                   workmem);

  free(workmem);

  *compressed_length = (size_t) size + 4;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("brieflz", name) == 0) {
    impl->get_max_compressed_size = squash_brieflz_get_max_compressed_size;
    impl->decompress_buffer = squash_brieflz_decompress_buffer;
    impl->compress_buffer_unsafe = squash_brieflz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
