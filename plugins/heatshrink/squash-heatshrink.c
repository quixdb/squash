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

typedef struct SquashHeatshrinkStream_s {
  SquashStream base_object;

  union {
    heatshrink_encoder* comp;
    heatshrink_decoder* decomp;
  } ctx;
} SquashHeatshrinkStream;

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

static void                    squash_heatshrink_stream_init     (SquashHeatshrinkStream* stream,
                                                                  SquashCodec* codec,
                                                                  SquashStreamType stream_type,
                                                                  SquashOptions* options,
                                                                  SquashDestroyNotify destroy_notify);
static SquashHeatshrinkStream* squash_heatshrink_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void                    squash_heatshrink_stream_destroy  (void* stream);

static SquashHeatshrinkStream*
squash_heatshrink_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashHeatshrinkStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = squash_malloc (sizeof (SquashHeatshrinkStream));
  if (SQUASH_UNLIKELY(stream == NULL))
    return (squash_error (SQUASH_MEMORY), NULL);

  squash_heatshrink_stream_init (stream, codec, stream_type, options, squash_heatshrink_stream_destroy);

  const uint8_t window_size = (uint8_t) squash_codec_get_option_int_index (codec, options, SQUASH_HEATSHRINK_OPT_WINDOW_SIZE);
  const uint8_t lookahead_size = (uint8_t) squash_codec_get_option_int_index (codec, options, SQUASH_HEATSHRINK_OPT_LOOKAHEAD_SIZE);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    stream->ctx.comp = heatshrink_encoder_alloc (window_size, lookahead_size);
    if (SQUASH_UNLIKELY(stream->ctx.comp == NULL)) {
      squash_object_unref (stream);
      return (squash_error (SQUASH_MEMORY), NULL);
    }
  } else {
    stream->ctx.decomp = heatshrink_decoder_alloc (256, window_size, lookahead_size);
    if (SQUASH_UNLIKELY(stream->ctx.decomp == NULL)) {
      squash_object_unref (stream);
      return (squash_error (SQUASH_MEMORY), NULL);
    }
  }

  return stream;
}

static void
squash_heatshrink_stream_init (SquashHeatshrinkStream* stream,
                               SquashCodec* codec,
                               SquashStreamType stream_type,
                               SquashOptions* options,
                               SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);
}

static void
squash_heatshrink_stream_destroy (void* stream) {
  const SquashHeatshrinkStream* s = (SquashHeatshrinkStream*) stream;

  if (s->base_object.stream_type == SQUASH_STREAM_COMPRESS)
    heatshrink_encoder_free (s->ctx.comp);
  else
    heatshrink_decoder_free (s->ctx.decomp);

  squash_stream_destroy (stream);
}

static SquashStream*
squash_heatshrink_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_heatshrink_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_heatshrink_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashHeatshrinkStream* s = (SquashHeatshrinkStream*) stream;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    HSE_poll_res hsp;
    HSE_sink_res hss;
    size_t processed;

    assert (stream->avail_out != 0);

    hsp = heatshrink_encoder_poll (s->ctx.comp, stream->next_out, stream->avail_out, &processed);
    if (SQUASH_UNLIKELY(0 > hsp))
      return squash_error (SQUASH_FAILED);

    if (0 != processed) {
      stream->next_out += processed;
      stream->avail_out -= processed;
    }

    if (HSER_POLL_MORE == hsp || 0 == stream->avail_out) {
      return SQUASH_PROCESSING;
    } else if (SQUASH_OPERATION_FINISH == operation) {
      HSE_finish_res hsf = heatshrink_encoder_finish (s->ctx.comp);
      if (SQUASH_UNLIKELY(hsf < 0))
        return squash_error (SQUASH_FAILED);

      return (HSER_FINISH_MORE == hsf) ? SQUASH_PROCESSING : SQUASH_OK;
    }

    {
      if (stream->avail_in != 0) {
        hss = heatshrink_encoder_sink (s->ctx.comp, (uint8_t*) stream->next_in, stream->avail_in, &processed);
        if (SQUASH_UNLIKELY(0 > hss))
          return squash_error (SQUASH_FAILED);
        stream->next_in += processed;
        stream->avail_in -= processed;
      }

      hsp = heatshrink_encoder_poll (s->ctx.comp, stream->next_out, stream->avail_out, &processed);
      if (SQUASH_UNLIKELY(0 > hsp))
        return squash_error (SQUASH_FAILED);

      if (0 != processed) {
        stream->next_out += processed;
        stream->avail_out -= processed;
      } else if (SQUASH_OPERATION_FINISH == operation) {
        HSE_finish_res hsf = heatshrink_encoder_finish (s->ctx.comp);
        if (SQUASH_UNLIKELY(hsf < 0))
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

    hsp = heatshrink_decoder_poll (s->ctx.decomp, stream->next_out, stream->avail_out, &processed);
    if (SQUASH_UNLIKELY(0 > hsp))
      return squash_error (SQUASH_FAILED);

    if (0 != processed) {
      stream->next_out += processed;
      stream->avail_out -= processed;
    }

    if (HSDR_POLL_MORE == hsp) {
      return SQUASH_PROCESSING;
    } else if (SQUASH_OPERATION_FINISH == operation) {
      HSD_finish_res hsf = heatshrink_decoder_finish (s->ctx.decomp);
      if (SQUASH_UNLIKELY(hsf < 0))
        return squash_error (SQUASH_FAILED);

      return (HSDR_FINISH_MORE == hsf) ? SQUASH_PROCESSING : SQUASH_OK;
    }

    {
      if (stream->avail_in != 0) {
        hss = heatshrink_decoder_sink (s->ctx.decomp, (uint8_t*) stream->next_in, stream->avail_in, &processed);
        if (SQUASH_UNLIKELY(0 > hss))
          return squash_error (SQUASH_FAILED);
        stream->next_in += processed;
        stream->avail_in -= processed;
      }

      hsp = heatshrink_decoder_poll (s->ctx.decomp, stream->next_out, stream->avail_out, &processed);
      if (SQUASH_UNLIKELY(0 > hsp))
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

  squash_assert_unreachable ();
}

static size_t
squash_heatshrink_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + (uncompressed_size / 8) + 1;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  if (SQUASH_LIKELY(strcmp ("heatshrink", squash_codec_get_name (codec)) == 0)) {
    impl->options = squash_heatshrink_options;
    impl->create_stream = squash_heatshrink_create_stream;
    impl->process_stream = squash_heatshrink_process_stream;
    impl->get_max_compressed_size = squash_heatshrink_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
