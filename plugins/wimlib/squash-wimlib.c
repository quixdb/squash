/* Copyright (c) 2016 The Squash Authors
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
#include <errno.h>

#include <squash/squash.h>

#include <wimlib.h>

enum SquashWimlibOptIndex {
  SQUASH_WIMLIB_OPT_LEVEL = 0,
  SQUASH_WIMLIB_OPT_BLOCK_SIZE
};

static SquashOptionInfo squash_wimlib_lzms_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 10 },
    .default_value.int_value = 5 },
  { "block-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 16 },
    .default_value.int_value = 16 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static SquashOptionInfo squash_wimlib_lzx_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 10 },
    .default_value.int_value = 5 },
  { "block-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 21 },
    .default_value.int_value = 16 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static SquashOptionInfo squash_wimlib_xpress_huffman_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 10 },
    .default_value.int_value = 5 },
  { "block-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 30 },
    .default_value.int_value = 20 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec       (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_wimlib_lzms_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 4096;
}

static size_t
squash_wimlib_lzx_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 256;
}

static size_t
squash_wimlib_xpress_huffman_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size * 2 + 256;
}

static enum wimlib_compression_type
squash_wimlib_compression_type_from_codec (SquashCodec* codec) {
  const char* codec_name = squash_codec_get_name (codec);

  switch (codec_name[2]) {
    case 'm':
      return WIMLIB_COMPRESSION_TYPE_LZMS;
    case 'x':
      return WIMLIB_COMPRESSION_TYPE_LZX;
    case 'r':
      return WIMLIB_COMPRESSION_TYPE_XPRESS;
      break;
    default:
      squash_assert_unreachable ();
  }
}

static SquashStatus
squash_wimlib_compress_buffer (SquashCodec* codec,
                               size_t* compressed_size,
                               uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                               size_t uncompressed_size,
                               const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                               SquashOptions* options) {
  struct wimlib_compressor* compressor = NULL;
  const unsigned int level = ((unsigned int) squash_options_get_int_at (options, codec, SQUASH_WIMLIB_OPT_LEVEL)) * 10U;
  const size_t max_block_size = ((size_t) 1) << squash_options_get_int_at (options, codec, SQUASH_WIMLIB_OPT_BLOCK_SIZE);

  {
    const int r =
      wimlib_create_compressor (squash_wimlib_compression_type_from_codec (codec),
                                max_block_size,
                                level,
                                &compressor);

    if (SQUASH_UNLIKELY(r != 0)) {
      switch (r) {
        case WIMLIB_ERR_NOMEM:
          return squash_error (SQUASH_MEMORY);
        default:
          return squash_error (SQUASH_FAILED);
      }
    }
  }

  const size_t os =
    wimlib_compress (uncompressed, uncompressed_size,
                     compressed, *compressed_size,
                     compressor);
  wimlib_free_compressor (compressor);

  fprintf (stderr, "%s:%d: %zu -> %zu of %zu\n", __FILE__, __LINE__, uncompressed_size, os, *compressed_size);

  if (SQUASH_UNLIKELY(os == 0))
    return squash_error (SQUASH_BUFFER_FULL);

  *compressed_size = os;

  return SQUASH_OK;
}

static SquashStatus
squash_wimlib_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_size,
                                 uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                 size_t compressed_size,
                                 const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                 SquashOptions* options) {
  struct wimlib_decompressor* decompressor = NULL;
  const size_t max_block_size = ((size_t) 1) << squash_options_get_int_at (options, codec, SQUASH_WIMLIB_OPT_BLOCK_SIZE);

  {
    const int r =
      wimlib_create_decompressor (squash_wimlib_compression_type_from_codec (codec),
                                  max_block_size,
                                  &decompressor);

    if (SQUASH_UNLIKELY(r != 0)) {
      switch (r) {
        case WIMLIB_ERR_NOMEM:
          return squash_error (SQUASH_MEMORY);
        default:
          return squash_error (SQUASH_FAILED);
      }
    }
  }

  const size_t os =
    wimlib_decompress (compressed, compressed_size,
                       decompressed, *decompressed_size,
                       decompressor);
  wimlib_free_decompressor (decompressor);

  fprintf (stderr, "%s:%d: %zu -> %zu of %zu\n", __FILE__, __LINE__, compressed_size, os, *decompressed_size);

  if (SQUASH_UNLIKELY(os != 0))
    return squash_error (SQUASH_BUFFER_FULL);

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lzms", name) == 0) {
    impl->options = squash_wimlib_lzms_options;
  impl->get_max_compressed_size = squash_wimlib_lzms_get_max_compressed_size;
  } else if (strcmp ("lzx", name) == 0) {
    impl->options = squash_wimlib_lzx_options;
  impl->get_max_compressed_size = squash_wimlib_lzx_get_max_compressed_size;
  } else if (strcmp ("xpress-huffman", name) == 0) {
    impl->options = squash_wimlib_xpress_huffman_options;
  impl->get_max_compressed_size = squash_wimlib_xpress_huffman_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  impl->decompress_buffer = squash_wimlib_decompress_buffer;
  impl->compress_buffer = squash_wimlib_compress_buffer;

  return SQUASH_OK;
}
