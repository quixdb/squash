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

#include "lzham/include/lzham.h"

typedef union {
  struct {
    lzham_compress_state_ptr ctx;
    lzham_compress_params params;
  } comp;
  struct {
    lzham_decompress_state_ptr ctx;
      lzham_decompress_params params;
  } decomp;
} SquashLZHAMStream;

enum SquashLZHAMOptIndex {
  SQUASH_LZHAM_OPT_LEVEL = 0,
  SQUASH_LZHAM_OPT_EXTREME_PARSING,
  SQUASH_LZHAM_OPT_DETERMINISTIC_PARSING,
  SQUASH_LZHAM_OPT_DECOMPRESSION_RATE_FOR_RATIO,
  SQUASH_LZHAM_OPT_DICT_SIZE_LOG2,
  SQUASH_LZHAM_OPT_UPDATE_RATE,
  SQUASH_LZHAM_OPT_UPDATE_INTERVAL
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
  { "dict-size-log2",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = LZHAM_MIN_DICT_SIZE_LOG2,
#if defined(__amd64__) || defined(_M_X64) || defined(__aarch64__) || defined(__ia64__) || defined(_M_IA64)
      .max = LZHAM_MAX_DICT_SIZE_LOG2_X64,
#else
      .max = LZHAM_MAX_DICT_SIZE_LOG2_X86,
#endif
      .allow_zero = true },
    .default_value.int_value = LZHAM_MAX_DICT_SIZE_LOG2_X86 },
  { "update-rate",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = LZHAM_SLOWEST_TABLE_UPDATE_RATE,
      .max = LZHAM_FASTEST_TABLE_UPDATE_RATE },
    .default_value.int_value = LZHAM_DEFAULT_TABLE_UPDATE_RATE },
  { "update-interval",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 12,
      .max = 128 },
    .default_value.int_value = 64 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecImpl* impl);

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
    .m_dict_size_log2                  = squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_DICT_SIZE_LOG2),
    .m_level                           = (lzham_compress_level) squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_LEVEL),
    .m_table_update_rate               = squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_UPDATE_RATE),
    .m_max_helper_threads              = -1,
    .m_compress_flags                  =
      squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_EXTREME_PARSING) ?
        LZHAM_COMP_FLAG_EXTREME_PARSING : 0 |
      squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_DETERMINISTIC_PARSING) ?
        LZHAM_COMP_FLAG_DETERMINISTIC_PARSING : 0 |
      squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_DECOMPRESSION_RATE_FOR_RATIO) ?
        LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO : 0,
    .m_num_seed_bytes                  = 0,
    .m_pSeed_bytes                     = NULL,
    .m_table_max_update_interval       = squash_options_get_int_at (options, codec, SQUASH_LZHAM_OPT_UPDATE_INTERVAL),
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

static bool
squash_lzham_init_stream (SquashStream* stream,
                          SquashStreamType stream_type,
                          SquashOptions* options,
                          void* priv) {
  SquashLZHAMStream* s = priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    squash_lzham_compress_apply_options (stream->codec, &(s->comp.params), options);
    s->comp.ctx = lzham_compress_init (&(s->comp.params));
  } else {
    squash_lzham_decompress_apply_options (stream->codec, &(s->decomp.params), options);
    s->decomp.ctx = lzham_decompress_init (&(s->decomp.params));
  }

  return true;
}

static void
squash_lzham_destroy_stream (SquashStream* stream, void* priv) {
  SquashLZHAMStream* s = priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    lzham_compress_deinit (s->comp.ctx);
  } else {
    lzham_decompress_deinit (s->decomp.ctx);
  }

  squash_stream_destroy (stream);
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
      HEDLEY_UNREACHABLE ();
      break;
  }

  HEDLEY_UNREACHABLE();
}

static SquashStatus
squash_lzham_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  SquashLZHAMStream* s = priv;
  SquashStatus res = SQUASH_FAILED;

  size_t input_size = stream->avail_in;
  size_t output_size = stream->avail_out;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    lzham_compress_status_t status;

    status = lzham_compress2 (s->comp.ctx,
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

    status = lzham_decompress (s->comp.ctx,
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
  /* Based on tests, it seems like the overhead is 5 bytes, plus an
     additional 5 bytes per 1/2 MiB or fraction thereof. */
  const size_t block_size = 1024 * 512;
  const size_t blocks = (uncompressed_size / block_size) + (((uncompressed_size % block_size) != 0) ? 1 : 0);
  return uncompressed_size + 5 + (5 * blocks);
}

static SquashStatus
squash_lzham_compress_buffer (SquashCodec* codec,
                              size_t* compressed_size,
                              uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                              size_t uncompressed_size,
                              const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                              SquashOptions* options) {
  lzham_compress_status_t status;
  lzham_compress_params params;

  squash_lzham_compress_apply_options (codec, &params, options);

  status = lzham_compress_memory (&params,
                                  compressed, compressed_size,
                                  uncompressed, uncompressed_size,
                                  NULL);

  if (HEDLEY_UNLIKELY(status != LZHAM_COMP_STATUS_SUCCESS)) {
    switch ((int) status) {
      case LZHAM_COMP_STATUS_INVALID_PARAMETER:
        return squash_error (SQUASH_BAD_VALUE);
      case LZHAM_COMP_STATUS_OUTPUT_BUF_TOO_SMALL:
        return squash_error (SQUASH_BUFFER_FULL);
      default:
        return squash_error (SQUASH_FAILED);
    }
  }

  return SQUASH_OK;
}

static SquashStatus
squash_lzham_decompress_buffer (SquashCodec* codec,
                                size_t* decompressed_size,
                                uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                                size_t compressed_size,
                                const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
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

  HEDLEY_UNREACHABLE ();
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  if (HEDLEY_LIKELY(strcmp ("lzham", squash_codec_get_name (codec)) == 0)) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->options = squash_lzham_options;
    impl->priv_size = sizeof(SquashLZHAMStream);
    impl->init_stream = squash_lzham_init_stream;
    impl->destroy_stream = squash_lzham_destroy_stream;
    impl->process_stream = squash_lzham_process_stream;
    impl->get_max_compressed_size = squash_lzham_get_max_compressed_size;
    impl->decompress_buffer = squash_lzham_decompress_buffer;
    impl->compress_buffer = squash_lzham_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
