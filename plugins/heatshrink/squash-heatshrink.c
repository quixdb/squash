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

#include "heatshrink/heatshrink_encoder.h"
#include "heatshrink/heatshrink_decoder.h"

typedef union {
  heatshrink_encoder* comp;
  heatshrink_decoder* decomp;
} SquashHeatshrinkCtx;

enum SquashHeatshrinkOptIndex {
  SQUASH_HEATSHRINK_OPT_WINDOW_SIZE = 0,
  SQUASH_HEATSHRINK_OPT_LOOKAHEAD_SIZE
};

static SquashOptionInfo squash_heatshrink_options[] = {
  { "window-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 4,
      .max = 15 },
    .default_value.int_value = 11 },
  { "lookahead-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 3,
      .max = 14 },
    .default_value.int_value = 4 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus                   squash_plugin_init_codec          (SquashCodec* codec, SquashCodecImpl* impl);


static bool
squash_heatshrink_init_stream (SquashStream* stream, SquashStreamType stream_type, SquashOptions* options, void* priv) {
  SquashHeatshrinkCtx* ctx = priv;
  SquashCodec* codec = stream->codec;

  const uint8_t window_size = (uint8_t) squash_options_get_int_at (options, codec, SQUASH_HEATSHRINK_OPT_WINDOW_SIZE);
  const uint8_t lookahead_size = (uint8_t) squash_options_get_int_at (options, codec, SQUASH_HEATSHRINK_OPT_LOOKAHEAD_SIZE);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    ctx->comp = heatshrink_encoder_alloc (window_size, lookahead_size);
    if (HEDLEY_UNLIKELY(ctx->comp == NULL))
      return (squash_error (SQUASH_MEMORY), false);
  } else {
    ctx->decomp = heatshrink_decoder_alloc (256, window_size, lookahead_size);
    if (HEDLEY_UNLIKELY(ctx->decomp == NULL))
      return (squash_error (SQUASH_MEMORY), false);
  }

  return true;
}

static void
squash_heatshrink_destroy_stream (SquashStream* stream, void* priv) {
  SquashHeatshrinkCtx* ctx = priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS)
    heatshrink_encoder_free (ctx->comp);
  else
    heatshrink_decoder_free (ctx->decomp);
}

static SquashStatus
squash_heatshrink_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  SquashHeatshrinkCtx* ctx = priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    HSE_poll_res hsp;
    HSE_sink_res hss;
    size_t processed;

    assert (stream->avail_out != 0);

    hsp = heatshrink_encoder_poll (ctx->comp, stream->next_out, stream->avail_out, &processed);
    if (HEDLEY_UNLIKELY(0 > hsp))
      return squash_error (SQUASH_FAILED);

    if (0 != processed) {
      stream->next_out += processed;
      stream->avail_out -= processed;
    }

    if (HSER_POLL_MORE == hsp || 0 == stream->avail_out) {
      return SQUASH_PROCESSING;
    } else if (SQUASH_OPERATION_FINISH == operation) {
      HSE_finish_res hsf = heatshrink_encoder_finish (ctx->comp);
      if (HEDLEY_UNLIKELY(hsf < 0))
        return squash_error (SQUASH_FAILED);

      return (HSER_FINISH_MORE == hsf) ? SQUASH_PROCESSING : SQUASH_OK;
    }

    {
      if (stream->avail_in != 0) {
        hss = heatshrink_encoder_sink (ctx->comp, (uint8_t*) stream->next_in, stream->avail_in, &processed);
        if (HEDLEY_UNLIKELY(0 > hss))
          return squash_error (SQUASH_FAILED);
        stream->next_in += processed;
        stream->avail_in -= processed;
      }

      hsp = heatshrink_encoder_poll (ctx->comp, stream->next_out, stream->avail_out, &processed);
      if (HEDLEY_UNLIKELY(0 > hsp))
        return squash_error (SQUASH_FAILED);

      if (0 != processed) {
        stream->next_out += processed;
        stream->avail_out -= processed;
      } else if (SQUASH_OPERATION_FINISH == operation) {
        HSE_finish_res hsf = heatshrink_encoder_finish (ctx->comp);
        if (HEDLEY_UNLIKELY(hsf < 0))
          return squash_error (SQUASH_FAILED);

        return (HSER_FINISH_MORE == hsf) ? SQUASH_PROCESSING : SQUASH_OK;
      }

      if (HSER_POLL_MORE == hsp)
        return SQUASH_PROCESSING;
    }

    if (0 == stream->avail_in) {
      return SQUASH_OK;
    } else {
      return SQUASH_PROCESSING;
    }
  } else {
    HSD_poll_res hsp;
    HSD_sink_res hss;
    size_t processed;

    assert (stream->avail_out != 0);

    hsp = heatshrink_decoder_poll (ctx->decomp, stream->next_out, stream->avail_out, &processed);
    if (HEDLEY_UNLIKELY(0 > hsp))
      return squash_error (SQUASH_FAILED);

    if (0 != processed) {
      stream->next_out += processed;
      stream->avail_out -= processed;
    }

    if (HSDR_POLL_MORE == hsp) {
      return SQUASH_PROCESSING;
    } else if (SQUASH_OPERATION_FINISH == operation) {
      HSD_finish_res hsf = heatshrink_decoder_finish (ctx->decomp);
      if (HEDLEY_UNLIKELY(hsf < 0))
        return squash_error (SQUASH_FAILED);

      return (HSDR_FINISH_MORE == hsf) ? SQUASH_PROCESSING : SQUASH_OK;
    }

    {
      if (stream->avail_in != 0) {
        hss = heatshrink_decoder_sink (ctx->decomp, (uint8_t*) stream->next_in, stream->avail_in, &processed);
        if (HEDLEY_UNLIKELY(0 > hss))
          return squash_error (SQUASH_FAILED);
        stream->next_in += processed;
        stream->avail_in -= processed;
      }

      hsp = heatshrink_decoder_poll (ctx->decomp, stream->next_out, stream->avail_out, &processed);
      if (HEDLEY_UNLIKELY(0 > hsp))
        return squash_error (SQUASH_FAILED);

      if (0 != processed) {
        stream->next_out += processed;
        stream->avail_out -= processed;
      }

      if (HSDR_POLL_MORE == hsp)
        return SQUASH_PROCESSING;
    }

    if (0 == stream->avail_in) {
      return SQUASH_OK;
    } else {
      return SQUASH_PROCESSING;
    }
  }

  HEDLEY_UNREACHABLE ();
}

static size_t
squash_heatshrink_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + (uncompressed_size / 8) + 1;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  if (HEDLEY_LIKELY(strcmp ("heatshrink", squash_codec_get_name (codec)) == 0)) {
    impl->options = squash_heatshrink_options;
    impl->priv_size = sizeof(SquashHeatshrinkCtx);
    impl->init_stream = squash_heatshrink_init_stream;
    impl->destroy_stream = squash_heatshrink_destroy_stream;
    impl->process_stream = squash_heatshrink_process_stream;
    impl->get_max_compressed_size = squash_heatshrink_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
