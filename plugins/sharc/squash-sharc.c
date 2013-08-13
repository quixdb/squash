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

#include "sharc/src/sharc.h"
#include "squash-sharc-stream.h"

typedef struct SquashSharcOptions_s {
  SquashOptions base_object;

  sharc_byte level;
} SquashSharcOptions;

typedef struct SquashSharcPluginStream_s {
  SquashStream base_object;

  SquashSharcStream sharc_stream;
} SquashSharcPluginStream;

#define SQUASH_SHARC_DEFAULT_LEVEL SHARC_MODE_SINGLE_PASS;

SquashStatus                    squash_plugin_init_codec           (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                     squash_sharc_options_init          (SquashSharcOptions* options,
                                                                    SquashCodec* codec,
                                                                    SquashDestroyNotify destroy_notify);
static SquashSharcOptions*      squash_sharc_options_new           (SquashCodec* codec);
static void                     squash_sharc_options_destroy       (void* options);
static void                     squash_sharc_options_free          (void* options);

static void                     squash_sharc_plugin_stream_init    (SquashSharcPluginStream* stream,
                                                                    SquashCodec* codec,
                                                                    SquashStreamType stream_type,
                                                                    SquashSharcOptions* options,
                                                                    SquashDestroyNotify destroy_notify);
static SquashSharcPluginStream* squash_sharc_plugin_stream_new     (SquashCodec* codec,
                                                                    SquashStreamType stream_type,
                                                                    SquashSharcOptions* options);
static void                     squash_sharc_plugin_stream_destroy (void* stream);
static void                     squash_sharc_plugin_stream_free    (void* stream);

static void
squash_sharc_options_init (SquashSharcOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

	options->level = SQUASH_SHARC_DEFAULT_LEVEL;
}

static SquashSharcOptions*
squash_sharc_options_new (SquashCodec* codec) {
  SquashSharcOptions* options;

  options = (SquashSharcOptions*) malloc (sizeof (SquashSharcOptions));
  squash_sharc_options_init (options, codec, squash_sharc_options_free);

  return options;
}

static void
squash_sharc_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_sharc_options_free (void* options) {
  squash_sharc_options_destroy ((SquashSharcOptions*) options);
  free (options);
}

static SquashOptions*
squash_sharc_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_sharc_options_new (codec);
}

static SquashStatus
squash_sharc_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashSharcOptions* opts = (SquashSharcOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 2 ) {
      switch (level) {
        case 1:
          opts->level = SHARC_MODE_SINGLE_PASS;
          break;
        case 2:
          opts->level = SHARC_MODE_DUAL_PASS;
          break;
      }
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static void
squash_sharc_plugin_stream_init (SquashSharcPluginStream* stream,
                                 SquashCodec* codec,
                                 SquashStreamType stream_type,
                                 SquashSharcOptions* options,
                                 SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  squash_sharc_stream_init (&(stream->sharc_stream), stream_type == SQUASH_STREAM_COMPRESS ? SQUASH_SHARC_STREAM_COMPRESS : SQUASH_SHARC_STREAM_DECOMPRESS);
  if (options != NULL) {
    stream->sharc_stream.mode = options->level;
  }
}

static void
squash_sharc_plugin_stream_destroy (void* stream) {
  squash_sharc_stream_destroy (&(((SquashSharcPluginStream*) stream)->sharc_stream));

  squash_stream_destroy (stream);
}

static void
squash_sharc_plugin_stream_free (void* stream) {
  squash_sharc_plugin_stream_destroy (stream);
  free (stream);
}

static SquashSharcPluginStream*
squash_sharc_plugin_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashSharcOptions* options) {
  SquashSharcPluginStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashSharcPluginStream*) malloc (sizeof (SquashSharcPluginStream));
  squash_sharc_plugin_stream_init (stream, codec, stream_type, options, squash_sharc_plugin_stream_free);

  return stream;
}

static SquashStream*
squash_sharc_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_sharc_plugin_stream_new (codec, stream_type, (SquashSharcOptions*) options);
}

static SquashStatus
squash_sharc_stream_status_to_squash_status (SquashSharcStreamStatus status) {
  switch (status) {
    case SQUASH_SHARC_STREAM_OK:
    case SQUASH_SHARC_STREAM_PROCESSING:
    case SQUASH_SHARC_STREAM_END_OF_STREAM:
    case SQUASH_SHARC_STREAM_FAILED:
      return (SquashStatus) status;
    case SQUASH_SHARC_STREAM_STATE:
      return SQUASH_STATE;
    case SQUASH_SHARC_STREAM_MEMORY:
      return SQUASH_MEMORY;
    default:
      return SQUASH_FAILED;
  }
}

#define SQUASH_SHARC_PLUGIN_STREAM_COPY_TO_SHARC_STREAM(stream,sharc_stream) \
  sharc_stream->next_in = stream->next_in;                              \
  sharc_stream->avail_in = stream->avail_in;                            \
  sharc_stream->next_out = stream->next_out;                            \
  sharc_stream->avail_out = stream->avail_out

#define SQUASH_SHARC_PLUGIN_STREAM_COPY_FROM_SHARC_STREAM(stream,sharc_stream) \
  stream->next_in = sharc_stream->next_in;                              \
  stream->avail_in = sharc_stream->avail_in;                            \
  stream->next_out = sharc_stream->next_out;                            \
  stream->avail_out = sharc_stream->avail_out

static SquashStatus
squash_sharc_process_stream (SquashStream* stream) {
  SquashSharcStream* sharc_stream = &(((SquashSharcPluginStream*) stream)->sharc_stream);
  SquashSharcStreamStatus sharc_e;

  SQUASH_SHARC_PLUGIN_STREAM_COPY_TO_SHARC_STREAM(stream, sharc_stream);
  sharc_e = squash_sharc_stream_process (sharc_stream);
  SQUASH_SHARC_PLUGIN_STREAM_COPY_FROM_SHARC_STREAM(stream, sharc_stream);

  return squash_sharc_stream_status_to_squash_status (sharc_e);
}

static SquashStatus
squash_sharc_flush_stream (SquashStream* stream) {
  SquashSharcStream* sharc_stream = &(((SquashSharcPluginStream*) stream)->sharc_stream);
  SquashSharcStreamStatus sharc_e;

  SQUASH_SHARC_PLUGIN_STREAM_COPY_TO_SHARC_STREAM(stream, sharc_stream);
  sharc_e = squash_sharc_stream_flush (sharc_stream);
  SQUASH_SHARC_PLUGIN_STREAM_COPY_FROM_SHARC_STREAM(stream,sharc_stream);

  return squash_sharc_stream_status_to_squash_status (sharc_e);
}

static SquashStatus
squash_sharc_finish_stream (SquashStream* stream) {
  SquashSharcStream* sharc_stream = &(((SquashSharcPluginStream*) stream)->sharc_stream);
  SquashSharcStreamStatus sharc_e;

  SQUASH_SHARC_PLUGIN_STREAM_COPY_TO_SHARC_STREAM(stream, sharc_stream);
  sharc_e = squash_sharc_stream_finish (sharc_stream);
  SQUASH_SHARC_PLUGIN_STREAM_COPY_FROM_SHARC_STREAM(stream,sharc_stream);

  return squash_sharc_stream_status_to_squash_status (sharc_e);
}

static size_t
squash_sharc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length +
    sizeof(SHARC_GENERIC_HEADER) +
    (sizeof(SHARC_BLOCK_HEADER) * (uncompressed_length / (SHARC_MAX_BUFFER_SIZE))) +
    (((uncompressed_length % (SHARC_MAX_BUFFER_SIZE)) == 0) ? 0 : sizeof(SHARC_BLOCK_HEADER));
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcasecmp ("sharc", name) == 0) {
    funcs->create_options = squash_sharc_create_options;
    funcs->parse_option = squash_sharc_parse_option;
    funcs->create_stream = squash_sharc_create_stream;
    funcs->process_stream = squash_sharc_process_stream;
    funcs->flush_stream = squash_sharc_flush_stream;
    funcs->finish_stream = squash_sharc_finish_stream;
    funcs->get_max_compressed_size = squash_sharc_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
