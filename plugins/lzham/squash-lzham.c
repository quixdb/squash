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
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <squash/squash.h>

#include "lzham/include/lzham.h"

typedef struct SquashLZHAMOptions_s {
  SquashOptions base_object;

  lzham_compress_level level;
  lzham_compress_flags comp_flags;
  lzham_decompress_flags decomp_flags;

} SquashLZHAMOptions;

typedef struct SquashLZHAMStream_s {
  SquashStream base_object;

  union {
    struct {
      lzham_compress_state_ptr ctx;
      lzham_compress_params params;
    } comp;
    struct {
      lzham_decompress_state_ptr ctx;
      lzham_decompress_params params;
    } decomp;
  } lzham;
} SquashLZHAMStream;

SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                squash_lzham_options_init    (SquashLZHAMOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashLZHAMOptions* squash_lzham_options_new     (SquashCodec* codec);
static void                squash_lzham_options_destroy (void* options);
static void                squash_lzham_options_free    (void* options);

static void                squash_lzham_stream_init     (SquashLZHAMStream* stream,
                                                         SquashCodec* codec,
                                                         SquashStreamType stream_type,
                                                         SquashLZHAMOptions* options,
                                                         SquashDestroyNotify destroy_notify);
static SquashLZHAMStream*  squash_lzham_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashLZHAMOptions* options);
static void                squash_lzham_stream_destroy  (void* stream);
static void                squash_lzham_stream_free     (void* stream);

static void                squash_lzham_compress_apply_options   (lzham_compress_params* params, SquashLZHAMOptions* options);
static void                squash_lzham_decompress_apply_options (lzham_decompress_params* params, SquashLZHAMOptions* options);

static void
squash_lzham_compress_apply_options (lzham_compress_params* params, SquashLZHAMOptions* options) {
  static const lzham_compress_params defaults = {
    sizeof(lzham_compress_params),
    LZHAM_MAX_DICT_SIZE_LOG2_X86,
    LZHAM_COMP_LEVEL_DEFAULT,
    LZHAM_DEFAULT_TABLE_UPDATE_RATE,
    -1, 0, 0, NULL, 0, 0 };

  *params = defaults;

  if (options != NULL) {
    params->m_level = options->level;
    params->m_compress_flags = options->comp_flags;
  }
}

static void
squash_lzham_decompress_apply_options (lzham_decompress_params* params, SquashLZHAMOptions* options) {
  static const lzham_decompress_params defaults = {
    sizeof(lzham_decompress_params),
    LZHAM_MAX_DICT_SIZE_LOG2_X86,
    LZHAM_DEFAULT_TABLE_UPDATE_RATE,
    0, 0, NULL, 0, 0 };

  *params = defaults;

  if (options != NULL) {
    params->m_decompress_flags = options->decomp_flags;
  }
}

static void
squash_lzham_options_init (SquashLZHAMOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->level = LZHAM_COMP_LEVEL_DEFAULT;
  options->comp_flags = 0;
  options->decomp_flags = 0;
}

static SquashLZHAMOptions*
squash_lzham_options_new (SquashCodec* codec) {
  SquashLZHAMOptions* options;

  options = (SquashLZHAMOptions*) malloc (sizeof (SquashLZHAMOptions));
  squash_lzham_options_init (options, codec, squash_lzham_options_free);

  return options;
}

static void
squash_lzham_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_lzham_options_free (void* options) {
  squash_lzham_options_destroy ((SquashLZHAMOptions*) options);
  free (options);
}

static SquashOptions*
squash_lzham_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_lzham_options_new (codec);
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

#define SQUASH_LZHAM_SET_FLAG(location, flag, value)          \
  (location = value ? (location | flag) : (location & ~flag))

static SquashStatus
squash_lzham_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashLZHAMOptions* opts = (SquashLZHAMOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = (int) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= LZHAM_COMP_LEVEL_FASTEST && level <= LZHAM_COMP_LEVEL_UBER ) {
      opts->level = (lzham_compress_level) level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "extreme-parsing")) {
    bool res;
    if (string_to_bool(value, &res)) {
      SQUASH_LZHAM_SET_FLAG(opts->comp_flags, LZHAM_COMP_FLAG_EXTREME_PARSING, res);
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "deterministic-parsing")) {
    bool res;
    if (string_to_bool(value, &res)) {
      SQUASH_LZHAM_SET_FLAG(opts->comp_flags, LZHAM_COMP_FLAG_DETERMINISTIC_PARSING, res);
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "decompression-rate-for-ratio")) {
    bool res;
    if (string_to_bool(value, &res)) {
      SQUASH_LZHAM_SET_FLAG(opts->comp_flags, LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO, res);
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static SquashLZHAMStream*
squash_lzham_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashLZHAMOptions* options) {
  int lzham_e = 0;
  SquashLZHAMStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashLZHAMStream*) malloc (sizeof (SquashLZHAMStream));
  squash_lzham_stream_init (stream, codec, stream_type, options, squash_lzham_stream_free);

  return stream;
}

static void
squash_lzham_stream_init (SquashLZHAMStream* stream,
                          SquashCodec* codec,
                          SquashStreamType stream_type,
                          SquashLZHAMOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  if (stream->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
    squash_lzham_compress_apply_options (&(stream->lzham.comp.params), options);
    stream->lzham.comp.ctx = lzham_compress_init (&(stream->lzham.comp.params));
  } else {
    squash_lzham_decompress_apply_options (&(stream->lzham.decomp.params), options);
    stream->lzham.decomp.ctx = lzham_decompress_init (&(stream->lzham.decomp.params));
  }
}

static void
squash_lzham_stream_destroy (void* stream) {
  SquashLZHAMStream* s = (SquashLZHAMStream*) stream;

  if (s->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
    lzham_compress_deinit (s->lzham.comp.ctx);
  } else {
    lzham_decompress_deinit (s->lzham.decomp.ctx);
  }

  squash_stream_destroy (stream);
}

static void
squash_lzham_stream_free (void* stream) {
  squash_lzham_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_lzham_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_lzham_stream_new (codec, stream_type, (SquashLZHAMOptions*) options);
}

static SquashStatus
squash_lzham_process_stream_ex (SquashStream* stream, lzham_flush_t flush_type) {
  SquashLZHAMStream* s = (SquashLZHAMStream*) stream;
  SquashStatus res = SQUASH_FAILED;

  size_t input_size = stream->avail_in;
  size_t output_size = stream->avail_out;

  fprintf (stderr, "%s %zu @ %p -> %zu @ %p\n",
           (stream->stream_type == SQUASH_STREAM_COMPRESS) ? "ENC" : "DEC",
           stream->avail_in, stream->next_in,
           stream->avail_out, stream->next_out);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    lzham_compress_status_t status;

    status = lzham_compress2 (s->lzham.comp.ctx,
                              stream->next_in, &input_size,
                              stream->next_out, &output_size,
                              flush_type);

    switch (status) {
      case LZHAM_COMP_STATUS_NOT_FINISHED:
      case LZHAM_COMP_STATUS_HAS_MORE_OUTPUT:
        res = SQUASH_PROCESSING;
        break;
      case LZHAM_COMP_STATUS_NEEDS_MORE_INPUT:
      case LZHAM_COMP_STATUS_SUCCESS:
        res = SQUASH_OK;
        break;
      default:
        fprintf (stderr, "o_o %d\n", status);
        res = SQUASH_FAILED;
        break;
    }
  } else {
    lzham_decompress_status_t status;

    status = lzham_decompress (s->lzham.comp.ctx,
                               stream->next_in, &input_size,
                               stream->next_out, &output_size,
                               (flush_type == LZHAM_FINISH && input_size == 0));

    switch (status) {
      case LZHAM_DECOMP_STATUS_NOT_FINISHED:
      case LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT:
        res = SQUASH_PROCESSING;
        break;
      case LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT:
      case LZHAM_DECOMP_STATUS_SUCCESS:
        res = SQUASH_OK;
        break;
      default:
        fprintf (stderr, "o_o %d\n", status);
        res = SQUASH_FAILED;
        break;
    }
  }

  stream->next_in   += input_size;
  stream->avail_in  -= input_size;
  stream->next_out  += output_size;
  stream->avail_out -= output_size;

  fprintf (stderr, "<<< %zu @ %p -> %zu @ %p\n",
           stream->avail_in, stream->next_in,
           stream->avail_out, stream->next_out);

  fprintf (stderr, "::> %d (%s)\n", res, squash_status_to_string (res));

  return res;
}

static SquashStatus
squash_lzham_process_stream (SquashStream* stream) {
  return squash_lzham_process_stream_ex (stream, LZHAM_NO_FLUSH);
}

static SquashStatus
squash_lzham_flush_stream (SquashStream* stream) {
  return squash_lzham_process_stream_ex (stream, LZHAM_SYNC_FLUSH);
}

static SquashStatus
squash_lzham_finish_stream (SquashStream* stream) {
  return squash_lzham_process_stream_ex (stream, LZHAM_FINISH);
}

static size_t
squash_lzham_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + 10;
}

static SquashStatus
squash_lzham_compress_buffer (SquashCodec* codec,
                              uint8_t* compressed, size_t* compressed_length,
                              const uint8_t* uncompressed, size_t uncompressed_length,
                              SquashOptions* options) {
  SquashStatus res;
  lzham_compress_status_t status;
  lzham_compress_params params;

  squash_lzham_compress_apply_options (&params, (SquashLZHAMOptions*) options);

  status = lzham_compress_memory (&params,
                                  compressed, compressed_length,
                                  uncompressed, uncompressed_length,
                                  NULL);

  if (status != LZHAM_COMP_STATUS_SUCCESS) {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

static SquashStatus
squash_lzham_decompress_buffer (SquashCodec* codec,
                                uint8_t* decompressed, size_t* decompressed_length,
                                const uint8_t* compressed, size_t compressed_length,
                                SquashOptions* options) {
  SquashStatus res;
  lzham_decompress_status_t status;
  lzham_decompress_params params;

  squash_lzham_decompress_apply_options (&params, (SquashLZHAMOptions*) options);

  status = lzham_decompress_memory (&params,
                                    decompressed, decompressed_length,
                                    compressed, compressed_length,
                                    NULL);

  if (status != LZHAM_DECOMP_STATUS_SUCCESS) {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  if (strcmp ("lzham", squash_codec_get_name (codec)) == 0) {
    funcs->create_options = squash_lzham_create_options;
    funcs->parse_option = squash_lzham_parse_option;
    funcs->create_stream = squash_lzham_create_stream;
    funcs->process_stream = squash_lzham_process_stream;
    funcs->flush_stream = squash_lzham_flush_stream;
    funcs->finish_stream = squash_lzham_finish_stream;
    funcs->get_max_compressed_size = squash_lzham_get_max_compressed_size;
    /* funcs->decompress_buffer = squash_lzham_decompress_buffer; */
    /* funcs->compress_buffer = squash_lzham_compress_buffer; */
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
