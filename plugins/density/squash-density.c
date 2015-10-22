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
#include <errno.h>

#include <squash/squash.h>

#include "density/src/density_api.h"

#define SQUASH_DENSITY_INPUT_MULTIPLE 32

typedef enum {
  SQUASH_DENSITY_ACTION_INIT,
  SQUASH_DENSITY_ACTION_CONTINUE_OR_FINISH,
  SQUASH_DENSITY_ACTION_CONTINUE,
  SQUASH_DENSITY_ACTION_FINISH,
  SQUASH_DENSITY_ACTION_FINISHED
} SquashDensityAction;

static size_t
squash_density_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size +
    32 +
    ((uncompressed_size / 256) * 8) +
    (((uncompressed_size % 256) == 0) * 8);
}

enum SquashDensityOptIndex {
  SQUASH_DENSITY_OPT_LEVEL = 0,
  SQUASH_DENSITY_OPT_CHECKSUM
};

static SquashOptionInfo squash_density_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_ENUM_INT,
    .info.enum_int = {
      3, (const int []) { 1, 7, 9 } },
    .default_value.int_value = 1 },
  { "checksum",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

typedef struct SquashDensityStream_s {
  SquashStream base_object;

  density_stream* stream;
  SquashDensityAction next;
  DENSITY_STREAM_STATE state;

  uint8_t buffer[DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE];
  size_t buffer_size;
  size_t buffer_pos;
  bool buffer_active;

  uint8_t input_buffer[SQUASH_DENSITY_INPUT_MULTIPLE];
  size_t input_buffer_size;
  bool input_buffer_active;

  size_t active_input_size;

  bool output_invalid;
} SquashDensityStream;

SQUASH_PLUGIN_EXPORT
SquashStatus                 squash_plugin_init_codec      (SquashCodec* codec,
                                                            SquashCodecImpl* impl);

static void                  squash_density_stream_init     (SquashDensityStream* stream,
                                                             SquashCodec* codec,
                                                             SquashStreamType stream_type,
                                                             SquashOptions* options,
                                                             SquashDestroyNotify destroy_notify);
static SquashDensityStream*  squash_density_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void                  squash_density_stream_destroy  (void* stream);
static void                  squash_density_stream_free     (void* stream);

static SquashDensityStream*
squash_density_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashDensityStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashDensityStream*) malloc (sizeof (SquashDensityStream));
  squash_density_stream_init (stream, codec, stream_type, options, squash_density_stream_free);

  return stream;
}

static void
squash_density_stream_init (SquashDensityStream* stream,
                            SquashCodec* codec,
                            SquashStreamType stream_type,
                            SquashOptions* options,
                            SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  stream->stream = density_stream_create (NULL, NULL);
  stream->next = SQUASH_DENSITY_ACTION_INIT;

  stream->buffer_size = 0;
  stream->buffer_pos = 0;
  stream->buffer_active = false;

  stream->input_buffer_size = 0;
  stream->input_buffer_active = false;

  stream->active_input_size = 0;

  stream->output_invalid = false;
}

static void
squash_density_stream_destroy (void* stream) {
  SquashDensityStream* s = (SquashDensityStream*) stream;

  density_stream_destroy (s->stream);

  squash_stream_destroy (stream);
}

static void
squash_density_stream_free (void* stream) {
  squash_density_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_density_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_density_stream_new (codec, stream_type, options);
}

static DENSITY_COMPRESSION_MODE
squash_density_level_to_mode (int level) {
  switch (level) {
    case 1:
      return DENSITY_COMPRESSION_MODE_CHAMELEON_ALGORITHM;
    case 7:
      return DENSITY_COMPRESSION_MODE_CHEETAH_ALGORITHM;
    case 9:
      return DENSITY_COMPRESSION_MODE_LION_ALGORITHM;
    default:
      squash_assert_unreachable ();
  }
}

static bool
squash_density_flush_internal_buffer (SquashStream* stream) {
  SquashDensityStream* s = (SquashDensityStream*) stream;
  const size_t buffer_remaining = s->buffer_size - s->buffer_pos;
  const size_t cp_size = (stream->avail_out < buffer_remaining) ? stream->avail_out : buffer_remaining;

  if (cp_size > 0) {
    memcpy (stream->next_out, s->buffer + s->buffer_pos, cp_size);
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;
    s->buffer_pos += cp_size;

    if (s->buffer_pos == s->buffer_size) {
      s->buffer_size = 0;
      s->buffer_pos = 0;
      return true;
    } else {
      return false;
    }
  }

  squash_assert_unreachable();
}

static size_t total_bytes_written = 0;

static SquashStatus
squash_density_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashDensityStream* s = (SquashDensityStream*) stream;

  if (s->buffer_size > 0) {
    squash_density_flush_internal_buffer (stream);
    return SQUASH_PROCESSING;
  }

  if (s->next == SQUASH_DENSITY_ACTION_INIT) {
    s->active_input_size = (stream->stream_type == SQUASH_STREAM_COMPRESS) ? ((stream->avail_in / SQUASH_DENSITY_INPUT_MULTIPLE) * SQUASH_DENSITY_INPUT_MULTIPLE) : stream->avail_in;
    if (stream->avail_out < DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE) {
      s->buffer_active = true;
      s->state = density_stream_prepare (s->stream, (uint8_t*) stream->next_in, s->active_input_size, s->buffer, DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE);
    } else {
      s->buffer_active = false;
      s->state = density_stream_prepare (s->stream, (uint8_t*) stream->next_in, s->active_input_size, stream->next_out, stream->avail_out);
    }
    if (s->state != DENSITY_STREAM_STATE_READY)
      return squash_error (SQUASH_FAILED);
  }

  switch (s->state) {
    case DENSITY_STREAM_STATE_STALL_ON_INPUT:
      if (s->input_buffer_size != 0 ||
          (stream->avail_in < SQUASH_DENSITY_INPUT_MULTIPLE && stream->stream_type == SQUASH_STREAM_COMPRESS && operation == SQUASH_OPERATION_PROCESS)) {
        const size_t remaining = SQUASH_DENSITY_INPUT_MULTIPLE - s->input_buffer_size;
        const size_t cp_size = remaining < stream->avail_in ? remaining : stream->avail_in;
        if (cp_size != 0) {
          memcpy (s->input_buffer + s->input_buffer_size, stream->next_in, cp_size);
          s->input_buffer_size += cp_size;
          stream->next_in += cp_size;
          stream->avail_in -= cp_size;
          assert (cp_size != 0);
        }
      }

      if (s->input_buffer_size != 0) {
        if (s->input_buffer_size == SQUASH_DENSITY_INPUT_MULTIPLE || operation != SQUASH_OPERATION_PROCESS) {
          s->active_input_size = s->input_buffer_size;
          s->input_buffer_active = true;
          density_stream_update_input (s->stream, s->input_buffer, s->input_buffer_size);
          s->state = DENSITY_STREAM_STATE_READY;
        } else {
          assert (stream->avail_in == 0);
          return SQUASH_OK;
        }
      } else {
        s->active_input_size = (stream->stream_type == SQUASH_STREAM_COMPRESS) ? ((stream->avail_in / SQUASH_DENSITY_INPUT_MULTIPLE) * SQUASH_DENSITY_INPUT_MULTIPLE) : stream->avail_in;
        density_stream_update_input (s->stream, stream->next_in, s->active_input_size);
        s->state = DENSITY_STREAM_STATE_READY;
      }
      break;
    case DENSITY_STREAM_STATE_STALL_ON_OUTPUT:
      {
        if (!s->output_invalid) {
          const size_t written = density_stream_output_available_for_use (s->stream);
          total_bytes_written += written;

          if (s->buffer_active) {
            s->buffer_size = written;
            s->buffer_pos = 0;

            const size_t cp_size = s->buffer_size < stream->avail_out ? s->buffer_size : stream->avail_out;
            memcpy (stream->next_out, s->buffer, cp_size);
            stream->next_out += cp_size;
            stream->avail_out -= cp_size;
            s->buffer_pos += cp_size;
            if (s->buffer_pos == s->buffer_size) {
              s->buffer_pos = 0;
              s->buffer_size = 0;
            }
          } else {
            assert (written <= stream->avail_out);
            stream->next_out += written;
            stream->avail_out -= written;
          }

          s->output_invalid = true;
          return SQUASH_PROCESSING;
        } else {
          if (stream->avail_out < DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE) {
            s->buffer_active = true;
            density_stream_update_output (s->stream, s->buffer, DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE);
          } else {
            s->buffer_active = false;
            density_stream_update_output (s->stream, stream->next_out, stream->avail_out);
          }
          s->output_invalid = false;
          s->state = DENSITY_STREAM_STATE_READY;
        }
      }
      break;
    case DENSITY_STREAM_STATE_READY:
      break;
    case DENSITY_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL:
    case DENSITY_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE:
    case DENSITY_STREAM_STATE_ERROR_INTEGRITY_CHECK_FAIL:
      return squash_error (SQUASH_FAILED);
  }

  assert (s->output_invalid == false);

  while (s->state == DENSITY_STREAM_STATE_READY && s->next != SQUASH_DENSITY_ACTION_FINISHED) {
    switch (s->next) {
      case SQUASH_DENSITY_ACTION_INIT:
        if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
          DENSITY_COMPRESSION_MODE compression_mode =
            squash_density_level_to_mode (squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_DENSITY_OPT_LEVEL));
          DENSITY_BLOCK_TYPE block_type =
            squash_codec_get_option_bool_index (stream->codec, stream->options, SQUASH_DENSITY_OPT_CHECKSUM) ?
              DENSITY_BLOCK_TYPE_WITH_HASHSUM_INTEGRITY_CHECK :
              DENSITY_BLOCK_TYPE_DEFAULT;

          s->state = density_stream_compress_init (s->stream, compression_mode, block_type);
        } else {
          s->state = density_stream_decompress_init (s->stream, NULL);
        }
        if (s->state != DENSITY_STREAM_STATE_READY)
          return squash_error (SQUASH_FAILED);
        s->next = SQUASH_DENSITY_ACTION_CONTINUE;
        break;
      case SQUASH_DENSITY_ACTION_CONTINUE_OR_FINISH:
        s->next = (operation == SQUASH_OPERATION_PROCESS) ? SQUASH_DENSITY_ACTION_CONTINUE : SQUASH_DENSITY_ACTION_FINISH;
        break;
      case SQUASH_DENSITY_ACTION_CONTINUE:
        if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
          s->state = density_stream_compress_continue (s->stream);
        } else {
          s->state = density_stream_decompress_continue (s->stream);
        }

        if (s->state == DENSITY_STREAM_STATE_STALL_ON_INPUT)
          s->next = SQUASH_DENSITY_ACTION_CONTINUE_OR_FINISH;

        break;
      case SQUASH_DENSITY_ACTION_FINISH:
        if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
          s->state = density_stream_compress_finish (s->stream);
        } else {
          s->state = density_stream_decompress_finish (s->stream);
        }
        if (s->state == DENSITY_STREAM_STATE_READY) {
          s->state = DENSITY_STREAM_STATE_STALL_ON_OUTPUT;
          s->output_invalid = false;
          s->next = SQUASH_DENSITY_ACTION_FINISHED;
        }
        break;
      case SQUASH_DENSITY_ACTION_FINISHED:
      default:
        squash_assert_unreachable();
        break;
    }
  }

  if (s->state == DENSITY_STREAM_STATE_STALL_ON_INPUT) {
    if (s->input_buffer_active) {
      assert (s->active_input_size == s->input_buffer_size);
      s->input_buffer_active = false;
      s->input_buffer_size = 0;
    } else {
      assert (s->active_input_size <= stream->avail_in);
      stream->next_in += s->active_input_size;
      stream->avail_in -= s->active_input_size;
    }
    s->active_input_size = 0;
  } else if (s->state == DENSITY_STREAM_STATE_STALL_ON_OUTPUT) {
    {
      if (!s->output_invalid) {
        const size_t written = density_stream_output_available_for_use (s->stream);
        total_bytes_written += written;

        if (s->buffer_active) {
          s->buffer_size = written;
          s->buffer_pos = 0;

          const size_t cp_size = s->buffer_size < stream->avail_out ? s->buffer_size : stream->avail_out;
          memcpy (stream->next_out, s->buffer, cp_size);
          stream->next_out += cp_size;
          stream->avail_out -= cp_size;
          s->buffer_pos += cp_size;
          if (s->buffer_pos == s->buffer_size) {
            s->buffer_pos = 0;
            s->buffer_size = 0;
          }
        } else {
          assert (written <= stream->avail_out);
          stream->next_out += written;
          stream->avail_out -= written;
        }

        s->output_invalid = true;
        return SQUASH_PROCESSING;
      } else {
        squash_assert_unreachable();
      }
    }
  }

  if (operation == SQUASH_OPERATION_FINISH)
    total_bytes_written = 0;

  if (stream->avail_in == 0) {
    return SQUASH_OK;
  } else {
    return SQUASH_PROCESSING;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("density", name) == 0) {
    impl->options = squash_density_options;
    impl->create_stream = squash_density_create_stream;
    impl->process_stream = squash_density_process_stream;
    impl->get_max_compressed_size = squash_density_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
