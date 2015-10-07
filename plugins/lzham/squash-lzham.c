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

enum SquashLZHAMOptIndex {
  SQUASH_LZHAM_OPT_LEVEL = 0,
  SQUASH_LZHAM_OPT_EXTREME_PARSING,
  SQUASH_LZHAM_OPT_DETERMINISTIC_PARSING,
  SQUASH_LZHAM_OPT_DECOMPRESSION_RATE_FOR_RATIO
};

static SquashOptionInfo squash_lzham_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 2 },
  { "extreme-parsing",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { "deterministic-parsing",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { "decompression-rate-for-ratio",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecImpl* impl);

static void                squash_lzham_stream_init     (SquashLZHAMStream* stream,
                                                         SquashCodec* codec,
                                                         SquashStreamType stream_type,
                                                         SquashOptions* options,
                                                         SquashDestroyNotify destroy_notify);
static SquashLZHAMStream*  squash_lzham_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void                squash_lzham_stream_destroy  (void* stream);
static void                squash_lzham_stream_free     (void* stream);

static void                squash_lzham_compress_apply_options   (SquashCodec* codec,
                                                                  lzham_compress_params* params,
                                                                  SquashOptions* options);
static void                squash_lzham_decompress_apply_options (SquashCodec* codec,
                                                                  lzham_decompress_params* params,
                                                                  SquashOptions* options);

static void
squash_lzham_compress_apply_options (SquashCodec* codec,
                                     lzham_compress_params* params,
                                     SquashOptions* options) {
  lzham_compress_params opts = {
    .m_struct_size                     = sizeof(lzham_compress_params),
    .m_dict_size_log2                  = LZHAM_MAX_DICT_SIZE_LOG2_X86,
    .m_level                           = squash_codec_get_option_int_index (codec, options, SQUASH_LZHAM_OPT_LEVEL),
    .m_table_update_rate               = LZHAM_DEFAULT_TABLE_UPDATE_RATE,
    .m_max_helper_threads              = -1,
    .m_compress_flags                  =
      squash_codec_get_option_int_index (codec, options, SQUASH_LZHAM_OPT_EXTREME_PARSING) ?
        LZHAM_COMP_FLAG_EXTREME_PARSING : 0 |
      squash_codec_get_option_int_index (codec, options, SQUASH_LZHAM_OPT_DETERMINISTIC_PARSING) ?
        LZHAM_COMP_FLAG_DETERMINISTIC_PARSING : 0 |
      squash_codec_get_option_int_index (codec, options, SQUASH_LZHAM_OPT_DECOMPRESSION_RATE_FOR_RATIO) ?
        LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO : 0,
    .m_num_seed_bytes                  = 0,
    .m_pSeed_bytes                     = NULL,
    .m_table_max_update_interval       = 0,
    .m_table_update_interval_slow_rate = 0
  };

  *params = opts;
}

static void
squash_lzham_decompress_apply_options (SquashCodec* codec,
                                       lzham_decompress_params* params,
                                       SquashOptions* options) {
  const lzham_decompress_params opts = {
    .m_struct_size                     = sizeof (lzham_decompress_params),
    .m_dict_size_log2                  = LZHAM_MAX_DICT_SIZE_LOG2_X86,
    .m_table_update_rate               = LZHAM_DEFAULT_TABLE_UPDATE_RATE,
    .m_decompress_flags                = 0,
    .m_num_seed_bytes                  = 0,
    .m_pSeed_bytes                     = NULL,
    .m_table_max_update_interval       = 0,
    .m_table_update_interval_slow_rate = 0
  };

  *params = opts;
}

static SquashLZHAMStream*
squash_lzham_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
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
                          SquashOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  if (stream->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
    squash_lzham_compress_apply_options (codec, &(stream->lzham.comp.params), options);
    stream->lzham.comp.ctx = lzham_compress_init (&(stream->lzham.comp.params));
  } else {
    squash_lzham_decompress_apply_options (codec, &(stream->lzham.decomp.params), options);
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
  return (SquashStream*) squash_lzham_stream_new (codec, stream_type, options);
}

static lzham_flush_t
squash_operation_to_lzham (SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return LZHAM_NO_FLUSH;
    case SQUASH_OPERATION_FLUSH:
      return LZHAM_SYNC_FLUSH;
    case SQUASH_OPERATION_FINISH:
      return LZHAM_FINISH;
    case SQUASH_OPERATION_TERMINATE:
      squash_assert_unreachable ();
      break;
  }

  squash_assert_unreachable();
}

static SquashStatus
squash_lzham_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashLZHAMStream* s = (SquashLZHAMStream*) stream;
  SquashStatus res = SQUASH_FAILED;

  size_t input_size = stream->avail_in;
  size_t output_size = stream->avail_out;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    lzham_compress_status_t status;

    status = lzham_compress2 (s->lzham.comp.ctx,
                              stream->next_in, &input_size,
                              stream->next_out, &output_size,
                              squash_operation_to_lzham (operation));

    switch ((int) status) {
      case LZHAM_COMP_STATUS_HAS_MORE_OUTPUT:
        res = SQUASH_PROCESSING;
        break;
      case LZHAM_COMP_STATUS_NOT_FINISHED:
      case LZHAM_COMP_STATUS_NEEDS_MORE_INPUT:
      case LZHAM_COMP_STATUS_SUCCESS:
        res = SQUASH_OK;
        break;
      default:
        res = SQUASH_FAILED;
        break;
    }
  } else {
    lzham_decompress_status_t status;

    status = lzham_decompress (s->lzham.comp.ctx,
                               stream->next_in, &input_size,
                               stream->next_out, &output_size,
                               (operation == SQUASH_OPERATION_FINISH && input_size == 0));

    switch ((int) status) {
      case LZHAM_DECOMP_STATUS_NOT_FINISHED:
      case LZHAM_DECOMP_STATUS_HAS_MORE_OUTPUT:
        res = SQUASH_PROCESSING;
        break;
      case LZHAM_DECOMP_STATUS_NEEDS_MORE_INPUT:
        res = (stream->avail_in > input_size) ? SQUASH_PROCESSING : SQUASH_OK;
        break;
      case LZHAM_DECOMP_STATUS_SUCCESS:
        res = SQUASH_OK;
        break;
      default:
        res = SQUASH_FAILED;
        break;
    }
  }

  stream->next_in   += input_size;
  stream->avail_in  -= input_size;
  stream->next_out  += output_size;
  stream->avail_out -= output_size;

  return res;
}

static size_t
squash_lzham_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 10;
}

static SquashStatus
squash_lzham_compress_buffer (SquashCodec* codec,
                              size_t* compressed_size,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  lzham_compress_status_t status;
  lzham_compress_params params;

  squash_lzham_compress_apply_options (codec, &params, options);

  status = lzham_compress_memory (&params,
                                  compressed, compressed_size,
                                  uncompressed, uncompressed_size,
                                  NULL);

  if (status != LZHAM_COMP_STATUS_SUCCESS) {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

static SquashStatus
squash_lzham_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                SquashOptions* options) {
  lzham_decompress_status_t status;
  lzham_decompress_params params;

  squash_lzham_decompress_apply_options (codec, &params, options);

  status = lzham_decompress_memory (&params,
                                    decompressed, decompressed_size,
                                    compressed, compressed_size,
                                    NULL);

  switch ((int) status) {
    case LZHAM_DECOMP_STATUS_SUCCESS:
      return SQUASH_OK;
    case LZHAM_DECOMP_STATUS_FAILED_DEST_BUF_TOO_SMALL:
      return squash_error (SQUASH_BUFFER_FULL);
    case LZHAM_DECOMP_STATUS_FAILED_EXPECTED_MORE_RAW_BYTES:
      return squash_error (SQUASH_BUFFER_EMPTY);
    default:
      return squash_error (SQUASH_FAILED);
  }

  squash_assert_unreachable ();
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  if (strcmp ("lzham", squash_codec_get_name (codec)) == 0) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->options = squash_lzham_options;
    impl->create_stream = squash_lzham_create_stream;
    impl->process_stream = squash_lzham_process_stream;
    impl->get_max_compressed_size = squash_lzham_get_max_compressed_size;
    impl->decompress_buffer = squash_lzham_decompress_buffer;
    impl->compress_buffer = squash_lzham_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
