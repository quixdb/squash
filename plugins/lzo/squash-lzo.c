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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <squash/squash.h>

#include <lzo/lzoconf.h>
#include <lzo/lzo1.h>
#include <lzo/lzo1a.h>
#include <lzo/lzo1b.h>
#include <lzo/lzo1c.h>
#include <lzo/lzo1f.h>
#include <lzo/lzo1x.h>
#include <lzo/lzo1y.h>
#include <lzo/lzo1z.h>

typedef struct _SquashLZOCompressor {
  int level;
  size_t work_mem;
  int(* compress) (const lzo_bytep src, lzo_uint src_len,
                   lzo_bytep dst, lzo_uintp dst_len,
                   lzo_voidp wrkmem);
} SquashLZOCompressor;

typedef struct _SquashLZOCodec {
  const char* name;
  size_t work_mem;
  int(* decompress) (const lzo_bytep src, lzo_uint src_len,
                     lzo_bytep dst, lzo_uintp dst_len,
                     lzo_voidp wrkmem);
  const SquashLZOCompressor* compressors;
} SquashLZOCodec;

typedef struct _SquashLZOOptions {
  SquashOptions base_object;

  const SquashLZOCompressor* compressor;
} SquashLZOOptions;

typedef struct _SquashLZOStream {
  SquashStream base_object;

  SquashLZOCodec* codec;
  SquashLZOCompressor* compressor;
} SquashLZOStream;

static const SquashLZOCompressor squash_lzo1_compressors[] = {
  { 1, LZO1_MEM_COMPRESS, lzo1_compress },
  { 99, LZO1_99_MEM_COMPRESS, lzo1_99_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1a_compressors[] = {
  { 1, LZO1A_MEM_COMPRESS, lzo1a_compress },
  { 99, LZO1A_99_MEM_COMPRESS, lzo1a_99_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1b_compressors[] = {
  { 1, LZO1B_MEM_COMPRESS, lzo1b_1_compress },
  { 2, LZO1B_MEM_COMPRESS, lzo1b_2_compress },
  { 3, LZO1B_MEM_COMPRESS, lzo1b_3_compress },
  { 4, LZO1B_MEM_COMPRESS, lzo1b_4_compress },
  { 5, LZO1B_MEM_COMPRESS, lzo1b_5_compress },
  { 6, LZO1B_MEM_COMPRESS, lzo1b_6_compress },
  { 7, LZO1B_MEM_COMPRESS, lzo1b_7_compress },
  { 8, LZO1B_MEM_COMPRESS, lzo1b_8_compress },
  { 9, LZO1B_MEM_COMPRESS, lzo1b_9_compress },
  { 99, LZO1B_99_MEM_COMPRESS, lzo1b_99_compress },
  { 999, LZO1B_999_MEM_COMPRESS, lzo1b_999_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1c_compressors[] = {
  { 1, LZO1C_MEM_COMPRESS, lzo1c_1_compress },
  { 2, LZO1C_MEM_COMPRESS, lzo1c_2_compress },
  { 3, LZO1C_MEM_COMPRESS, lzo1c_3_compress },
  { 4, LZO1C_MEM_COMPRESS, lzo1c_4_compress },
  { 5, LZO1C_MEM_COMPRESS, lzo1c_5_compress },
  { 6, LZO1C_MEM_COMPRESS, lzo1c_6_compress },
  { 7, LZO1C_MEM_COMPRESS, lzo1c_7_compress },
  { 8, LZO1C_MEM_COMPRESS, lzo1c_8_compress },
  { 9, LZO1C_MEM_COMPRESS, lzo1c_9_compress },
  { 99, LZO1C_99_MEM_COMPRESS, lzo1c_99_compress },
  { 999, LZO1C_999_MEM_COMPRESS, lzo1c_999_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1f_compressors[] = {
  { 1, LZO1F_MEM_COMPRESS, lzo1f_1_compress },
  { 999, LZO1F_999_MEM_COMPRESS, lzo1f_999_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1x_compressors[] = {
  { 1, LZO1X_1_MEM_COMPRESS, lzo1x_1_compress },
  { 11, LZO1X_1_11_MEM_COMPRESS, lzo1x_1_11_compress },
  { 12, LZO1X_1_12_MEM_COMPRESS, lzo1x_1_12_compress },
  { 15, LZO1X_1_15_MEM_COMPRESS, lzo1x_1_15_compress },
  { 999, LZO1X_999_MEM_COMPRESS, lzo1x_999_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1y_compressors[] = {
  { 1, LZO1Y_MEM_COMPRESS, lzo1y_1_compress },
  { 999, LZO1Y_999_MEM_COMPRESS, lzo1y_999_compress },
  { 0, }
};

static const SquashLZOCompressor squash_lzo1z_compressors[] = {
  /* { 1, LZO1Z_MEM_COMPRESS, lzo1z_1_compress }, */
  { 999, LZO1Z_999_MEM_COMPRESS, lzo1z_999_compress },
  { 0, }
};

static const SquashLZOCodec squash_lzo_codecs[] = {
  { "lzo1", LZO1_MEM_DECOMPRESS, lzo1_decompress, squash_lzo1_compressors },
  { "lzo1a", LZO1A_MEM_DECOMPRESS, lzo1a_decompress, squash_lzo1a_compressors },
  { "lzo1b", LZO1B_MEM_DECOMPRESS, lzo1b_decompress_safe, squash_lzo1b_compressors },
  { "lzo1c", LZO1C_MEM_DECOMPRESS, lzo1c_decompress_safe, squash_lzo1c_compressors },
  { "lzo1f", LZO1F_MEM_DECOMPRESS, lzo1f_decompress_safe, squash_lzo1f_compressors },
  { "lzo1x", LZO1X_MEM_DECOMPRESS, lzo1x_decompress_safe, squash_lzo1x_compressors },
  { "lzo1y", LZO1Y_MEM_DECOMPRESS, lzo1y_decompress_safe, squash_lzo1y_compressors },
  { "lzo1z", LZO1Z_MEM_DECOMPRESS, lzo1z_decompress_safe, squash_lzo1z_compressors },
  { NULL, }
};

static const SquashLZOCompressor*
squash_lzo_codec_get_compressor (const SquashLZOCodec* codec, int level) {
  const SquashLZOCompressor* compressor;

  for ( compressor = codec->compressors ;
        compressor->level != 0 ;
        compressor++ ) {
    if (compressor->level == level) {
      return compressor;
    }
  }

  return NULL;
}

static const SquashLZOCodec*
squash_lzo_codec_from_name (const char* name) {
  const SquashLZOCodec* codec;

  for ( codec = squash_lzo_codecs ;
        codec->name != NULL ;
        codec++ ) {
    if (strcmp (codec->name, name) == 0) {
      return codec;
    }
  }

  return NULL;
}

static const SquashStatus
squash_lzo_status_to_squash_status (int lzo_e) {
  SquashStatus res;

  switch (lzo_e) {
    case LZO_E_OK:
      res = SQUASH_OK;
      break;
    case LZO_E_OUT_OF_MEMORY:
      res = SQUASH_MEMORY;
      break;
    case LZO_E_INPUT_OVERRUN:
    case LZO_E_INPUT_NOT_CONSUMED:
      res = SQUASH_BUFFER_EMPTY;
      break;
    case LZO_E_OUTPUT_OVERRUN:
      res = SQUASH_BUFFER_FULL;
      break;
    case LZO_E_INVALID_ARGUMENT:
      res = SQUASH_BAD_VALUE;
      break;
    case LZO_E_NOT_COMPRESSIBLE:
    case LZO_E_ERROR:
    default:
      res = SQUASH_FAILED;
      break;
  }

  return res;
}

SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_lzo_options_init    (SquashLZOOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashLZOOptions* squash_lzo_options_new     (SquashCodec* codec);
static void              squash_lzo_options_destroy (void* options);
static void              squash_lzo_options_free    (void* options);

SquashStatus             squash_plugin_init         (SquashPlugin* plugin);
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void
squash_lzo_options_init (SquashLZOOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  const SquashLZOCodec* lzo_codec;

  assert (options != NULL);

  lzo_codec = squash_lzo_codec_from_name (squash_codec_get_name (codec));

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);
  options->compressor = &(lzo_codec->compressors[0]);
}

static SquashLZOOptions*
squash_lzo_options_new (SquashCodec* codec) {
  SquashLZOOptions* options;

  options = (SquashLZOOptions*) malloc (sizeof (SquashLZOOptions));
  squash_lzo_options_init (options, codec, squash_lzo_options_free);

  return options;
}

static void
squash_lzo_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_lzo_options_free (void* options) {
  squash_lzo_options_destroy ((SquashLZOOptions*) options);
  free (options);
}

static SquashOptions*
squash_lzo_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_lzo_options_new (codec);
}

static SquashStatus
squash_lzo_parse_option (SquashOptions* options, const char* key, const char* value) {
  const SquashLZOCodec* lzo_codec;
  const SquashLZOCompressor* compressor;
  SquashLZOOptions* opts = (SquashLZOOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = (int) strtol (value, &endptr, 0);

    if ( *endptr != '\0' ) {
      return SQUASH_BAD_VALUE;
    }

    lzo_codec = squash_lzo_codec_from_name (squash_codec_get_name (options->codec));
    compressor = squash_lzo_codec_get_compressor (lzo_codec, level);

    if ( compressor != NULL ) {
      opts->compressor = compressor;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static size_t
squash_lzo_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + uncompressed_length / 16 + 64 + 3;
}

static SquashStatus
squash_lzo_decompress_buffer (SquashCodec* codec,
                              uint8_t* decompressed, size_t* decompressed_length,
                              const uint8_t* compressed, size_t compressed_length,
                              SquashOptions* options) {
  const SquashLZOCodec* lzo_codec;
  const char* codec_name;
  int lzo_e;
  lzo_voidp work_mem = NULL;
  lzo_uint decompressed_len, compressed_len;

  assert (codec != NULL);
  codec_name = squash_codec_get_name (codec);
  assert (codec_name != NULL);
  lzo_codec = squash_lzo_codec_from_name (codec_name);

  if (lzo_codec->work_mem > 0) {
    work_mem = (lzo_voidp) malloc (lzo_codec->work_mem);
    if (work_mem == NULL) {
      return SQUASH_MEMORY;
    }
  }

  compressed_len = (lzo_uint) compressed_length;
  decompressed_len = (lzo_uint) *decompressed_length;
  lzo_e = lzo_codec->decompress (compressed, compressed_len,
                                 decompressed, &decompressed_len,
                                 work_mem);
  *decompressed_length = (size_t) decompressed_len;

  if (work_mem != NULL) {
    free (work_mem);
  }

  return squash_lzo_status_to_squash_status (lzo_e);
}

static SquashStatus
squash_lzo_compress_buffer (SquashCodec* codec,
                            uint8_t* compressed, size_t* compressed_length,
                            const uint8_t* uncompressed, size_t uncompressed_length,
                            SquashOptions* options) {
  const SquashLZOCodec* lzo_codec;
  const SquashLZOCompressor* compressor;
  SquashLZOOptions* opts = (SquashLZOOptions*) options;
  const char* codec_name;
  lzo_voidp work_mem = NULL;
  int lzo_e;
  lzo_uint uncompressed_len, compressed_len;

  assert (codec != NULL);
  codec_name = squash_codec_get_name (codec);
  assert (codec_name != NULL);
  lzo_codec = squash_lzo_codec_from_name (codec_name);
  assert (lzo_codec != NULL);

  compressor = (opts == NULL) ? &(lzo_codec->compressors[0]) : opts->compressor;

  if (compressor->work_mem > 0) {
    work_mem = (lzo_voidp) malloc (compressor->work_mem);
    if (work_mem == NULL) {
      return SQUASH_MEMORY;
    }
  }

  uncompressed_len = (lzo_uint) uncompressed_length;
  compressed_len = (lzo_uint) (*compressed_length);
  lzo_e = compressor->compress (uncompressed, uncompressed_len,
                                compressed, &compressed_len,
                                work_mem);
  *compressed_length = (size_t) compressed_len;

  if (work_mem != NULL) {
    free (work_mem);
  }

  return squash_lzo_status_to_squash_status (lzo_e);
}

SquashStatus
squash_plugin_init (SquashPlugin* plugin) {
  return squash_lzo_status_to_squash_status (lzo_init ());
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  if (squash_lzo_codec_from_name (squash_codec_get_name (codec)) != NULL) {
    funcs->create_options = squash_lzo_create_options;
    funcs->parse_option = squash_lzo_parse_option;
    funcs->get_max_compressed_size = squash_lzo_get_max_compressed_size;
    funcs->decompress_buffer = squash_lzo_decompress_buffer;
    funcs->compress_buffer = squash_lzo_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
