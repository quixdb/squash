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
read_varuint64 (const uint8_t *p, size_t p_size, uint64_t *v) {
  uint64_t n = 0;
  size_t i;

  for (i = 0; i < 8 && i < p_size && *p > 0x7F; ++i) {
    n = (n << 7) | (*p++ & 0x7F);
  }

  if (i == p_size) {
    return 0;
  }
  else if (i == 8) {
    n = (n << 8) | *p;
  }
  else {
    n = (n << 7) | *p;
  }

  *v = n;

  return i + 1;
}

static size_t
write_varuint64 (uint8_t *p, size_t p_size, uint64_t v) {
  uint8_t buf[10];
  size_t i;
  size_t j;

  if (v & 0xFF00000000000000ULL) {
    if (p_size < 9) {
      return 0;
    }

    p[8] = (uint8_t) v;
    v >>= 8;

    i = 7;

    for (j = 0; j < 8; ++j) {
      p[i--] = (uint8_t) ((v & 0x7F) | 0x80);
      v >>= 7;
    }

    return 9;
  }

  i = 0;

  buf[i++] = (uint8_t) (v & 0x7F);
  v >>= 7;

  while (v > 0) {
    buf[i++] = (uint8_t) ((v & 0x7F) | 0x80);
    v >>= 7;
  }

  if (i > p_size) {
    return 0;
  }

  for (j = 0; j < i; ++j) {
    p[j] = buf[i - j - 1];
  }

  return i;
}

static size_t
squash_brieflz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return (size_t) blz_max_packed_size ((unsigned long) uncompressed_length) + 9;
}

static size_t
squash_brieflz_get_uncompressed_size (SquashCodec* codec,
                                      size_t compressed_length,
                                      const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  uint64_t v;

  return read_varuint64 (compressed, compressed_length, &v) == 0 ? 0 : v;
}

static SquashStatus
squash_brieflz_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_length,
                                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                  size_t compressed_length,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                  SquashOptions* options) {
  const uint8_t *src = compressed;
  uint64_t original_size;
  size_t size;

  size = read_varuint64 (src, compressed_length, &original_size);

  if (size == 0) {
    return squash_error (SQUASH_FAILED);
  }

  src += size;
  compressed_length -= size;

  if (original_size > *decompressed_length) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  size = blz_depack_safe (src, (unsigned long) compressed_length,
                          decompressed, (unsigned long) original_size);

  if (size != original_size) {
    return squash_error (SQUASH_FAILED);
  }

  *decompressed_length = size;

  return SQUASH_OK;
}

static SquashStatus
squash_brieflz_compress_buffer (SquashCodec* codec,
                                size_t* compressed_length,
                                uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                size_t uncompressed_length,
                                const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                SquashOptions* options) {
  uint8_t *dst = compressed;
  void *workmem = NULL;
  size_t size;

  if ((unsigned long) *compressed_length
    < squash_brieflz_get_max_compressed_size (codec, uncompressed_length)) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  size = write_varuint64 (dst, *compressed_length, uncompressed_length);

  if (size == 0) {
    return squash_error (SQUASH_BUFFER_FULL);
  }

  dst += size;

  workmem = malloc (blz_workmem_size ((unsigned long) uncompressed_length));

  if (workmem == NULL) {
    return squash_error (SQUASH_MEMORY);
  }

  size += blz_pack (uncompressed, dst,
                    (unsigned long) uncompressed_length,
                    workmem);

  free(workmem);

  *compressed_length = size;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("brieflz", name) == 0) {
    impl->get_uncompressed_size = squash_brieflz_get_uncompressed_size;
    impl->get_max_compressed_size = squash_brieflz_get_max_compressed_size;
    impl->decompress_buffer = squash_brieflz_decompress_buffer;
    impl->compress_buffer_unsafe = squash_brieflz_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
