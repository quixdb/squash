/* Copyright (c) 2013-2015 The Squash Authors
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

#include <squash/squash.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "crush.h"

enum SquashCrushOptIndex {
  SQUASH_CRUSH_OPT_LEVEL = 0
};

static SquashOptionInfo squash_crush_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 2 },
    .default_value.int_value = 1 },
};

typedef struct SquashCrushStream_s {
  SquashStream base_object;

  SquashOperation operation;

  CrushContext ctx;
} SquashCrushStream;

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                squash_crush_stream_init     (SquashCrushStream* stream,
                                                         SquashCodec* codec,
                                                         SquashStreamType stream_type,
                                                         SquashOptions* options,
                                                         SquashDestroyNotify destroy_notify);
static SquashCrushStream*  squash_crush_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void                squash_crush_stream_destroy  (void* stream);
static void                squash_crush_stream_free     (void* stream);

static size_t
squash_crush_reader (void* buffer, size_t size, void* user_data) {
  uint8_t* buf = buffer;
  SquashCrushStream* stream = user_data;

  size_t remaining = size;
  while (remaining != 0) {
    const size_t cp_size = (stream->base_object.avail_in < remaining) ? stream->base_object.avail_in : remaining;

    memcpy (buf + (size - remaining), stream->base_object.next_in, cp_size);
    stream->base_object.next_in += cp_size;
    stream->base_object.avail_in -= cp_size;
    remaining -= cp_size;

    if (remaining != 0) {
      if (stream->operation == SQUASH_OPERATION_FINISH)
        break;

      stream->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_OK);
    }
  }

  return (size - remaining);
}

static size_t
squash_crush_writer (uint8_t* buf, size_t size, SquashCrushStream* stream) {
  size_t remaining = size;
  while (remaining != 0) {
    const size_t cp_size = (stream->base_object.avail_out < remaining) ? stream->base_object.avail_out : remaining;

    memcpy (stream->base_object.next_out, buf + (size - remaining), cp_size);
    stream->base_object.next_out += cp_size;
    stream->base_object.avail_out -= cp_size;
    remaining -= cp_size;

    if (remaining != 0) {
      stream->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_PROCESSING);
    }
  }

  return (size - remaining);
}

static void
squash_crush_stream_init (SquashCrushStream* stream,
                          SquashCodec* codec,
                          SquashStreamType stream_type,
                          SquashOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  crush_init(&(stream->ctx), squash_crush_reader, (CrushWriteFunc) squash_crush_writer, stream, NULL);
}

static SquashCrushStream*
squash_crush_stream_new (SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashOptions* options) {
  SquashCrushStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashCrushStream*) malloc (sizeof (SquashCrushStream));
  squash_crush_stream_init (stream, codec, stream_type, options, squash_crush_stream_free);

  return stream;
}

static void
squash_crush_stream_destroy (void* stream) {
  crush_destroy (&(((SquashCrushStream*) stream)->ctx));

  squash_stream_destroy (stream);
}

static void
squash_crush_stream_free (void* stream) {
  squash_crush_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_crush_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_crush_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_crush_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashCrushStream* s = (SquashCrushStream*) stream;
  int res;

  s->operation = operation;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    res = crush_compress (&(s->ctx), squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_CRUSH_OPT_LEVEL));
  } else {
    res = crush_decompress (&(s->ctx));
  }

  return (res == 0) ? SQUASH_OK : SQUASH_FAILED;
}

static size_t
squash_crush_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return
    uncompressed_length + 4 + (uncompressed_length / 7) + ((uncompressed_length % 7) == 0 ? 0 : 1);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("crush", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    funcs->options = squash_crush_options;
    funcs->create_stream = squash_crush_create_stream;
    funcs->process_stream = squash_crush_process_stream;
    funcs->get_max_compressed_size = squash_crush_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
