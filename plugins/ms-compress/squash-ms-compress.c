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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <squash/squash.h>

#include "ms-compress/include/mscomp.h"
#include "ms-compress/include/xpress_huff.h"

typedef struct SquashMSCompStream_s {
  SquashStream base_object;

  mscomp_stream mscomp;
} SquashMSCompStream;

SQUASH_PLUGIN_EXPORT
SquashStatus                squash_plugin_init_codec  (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                 squash_ms_stream_init     (SquashMSCompStream* stream,
                                                       SquashCodec* codec,
                                                       SquashStreamType stream_type,
                                                       SquashOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashMSCompStream*  squash_ms_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void                 squash_ms_stream_destroy  (void* stream);
static void                 squash_ms_stream_free     (void* stream);

static MSCompFormat
squash_ms_format_from_codec (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);

  if (name[5] == 0)
    return MSCOMP_LZNT1;
  else if (name[6] == 0)
    return MSCOMP_XPRESS;
  else if (name[14] == 0)
    return MSCOMP_XPRESS_HUFF;
  else
    assert (false);
}

static SquashStatus
squash_ms_status_to_squash_status (MSCompStatus status) {
  switch ((int) status) {
    case MSCOMP_OK:
      return SQUASH_OK;
    case MSCOMP_ERRNO:
      return SQUASH_FAILED;
    case MSCOMP_ARG_ERROR:
      return SQUASH_BAD_PARAM;
    case MSCOMP_DATA_ERROR:
      return SQUASH_FAILED;
    case MSCOMP_MEM_ERROR:
      return SQUASH_MEMORY;
    case MSCOMP_BUF_ERROR:
      return SQUASH_FAILED;
    default:
      return SQUASH_FAILED;
  }
}

static SquashMSCompStream*
squash_ms_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashMSCompStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = malloc (sizeof (SquashMSCompStream));
  squash_ms_stream_init (stream, codec, stream_type, options, squash_ms_stream_free);

  return stream;
}

static void
squash_ms_stream_init (SquashMSCompStream* stream,
                       SquashCodec* codec,
                       SquashStreamType stream_type,
                       SquashOptions* options,
                       SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, options, destroy_notify);

  MSCompStatus status;
  MSCompFormat format = squash_ms_format_from_codec (codec);
  if (stream->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
    status = ms_deflate_init (format, &(stream->mscomp));
  } else {
    status = ms_inflate_init (format, &(stream->mscomp));
  }
  assert (status == MSCOMP_OK);
}

static void
squash_ms_stream_destroy (void* stream) {
  SquashMSCompStream* s = (SquashMSCompStream*) stream;

  if (s->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
    ms_deflate_end(&(s->mscomp));
  } else {
    ms_inflate_end(&(s->mscomp));
  }

  squash_stream_destroy (stream);
}

static void
squash_ms_stream_free (void* stream) {
  squash_ms_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_ms_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_ms_stream_new (codec, stream_type, (SquashOptions*) options);
}

static MSCompFlush
squash_ms_comp_flush_from_operation (SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return MSCOMP_NO_FLUSH;
    case SQUASH_OPERATION_FLUSH:
      return MSCOMP_FLUSH;
    case SQUASH_OPERATION_FINISH:
      return MSCOMP_FINISH;
  }
  assert (false);
}

static SquashStatus
squash_ms_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashStatus status = SQUASH_FAILED;
  MSCompStatus res;
  SquashMSCompStream* s = (SquashMSCompStream*) stream;

  s->mscomp.in = stream->next_in;
  s->mscomp.in_avail = stream->avail_in;
  s->mscomp.out = stream->next_out;
  s->mscomp.out_avail = stream->avail_out;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    res = ms_deflate(&(s->mscomp), squash_ms_comp_flush_from_operation (operation));
  } else {
    res = ms_inflate(&(s->mscomp));
  }

  stream->next_in = s->mscomp.in;
  stream->avail_in = s->mscomp.in_avail;
  stream->next_out = s->mscomp.out;
  stream->avail_out = s->mscomp.out_avail;

  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      switch (operation) {
        case SQUASH_OPERATION_PROCESS:
          switch ((int) res) {
            case MSCOMP_OK:
              status = (stream->avail_in == 0) ? SQUASH_OK : SQUASH_PROCESSING;
              break;
            default:
              status = squash_ms_status_to_squash_status (res);
              break;
          }
          break;
        case SQUASH_OPERATION_FLUSH:
          switch ((int) res) {
            case MSCOMP_OK:
              status = SQUASH_OK;
              break;
            default:
              status = squash_ms_status_to_squash_status (res);
              break;
          }
          break;
        case SQUASH_OPERATION_FINISH:
          switch ((int) res) {
            case MSCOMP_OK:
              status = SQUASH_PROCESSING;
              break;
            case MSCOMP_STREAM_END:
              status = SQUASH_OK;
              break;
            default:
              status = squash_ms_status_to_squash_status (res);
              break;
          }
          break;
      }
      break;
    case SQUASH_STREAM_DECOMPRESS:
      switch (operation) {
        case SQUASH_OPERATION_PROCESS:
        case SQUASH_OPERATION_FLUSH:
        case SQUASH_OPERATION_FINISH:
          switch ((int) res) {
            case MSCOMP_OK:
            case MSCOMP_POSSIBLE_STREAM_END:
              status = (stream->avail_in == 0 && stream->avail_out > 0) ? SQUASH_OK : SQUASH_PROCESSING;
              break;
            default:
              status = squash_ms_status_to_squash_status (res);
              break;
          }
          break;
      }
      break;
  }

  return status;
}

static size_t
squash_ms_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return ms_max_compressed_size (squash_ms_format_from_codec (codec), uncompressed_length);
}

static SquashStatus
squash_ms_compress_buffer (SquashCodec* codec,
                           size_t* compressed_length,
                           uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                           size_t uncompressed_length,
                           const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                           SquashOptions* options) {
  MSCompStatus status = ms_compress (squash_ms_format_from_codec (codec),
                                     uncompressed, uncompressed_length, compressed, compressed_length);
  return squash_ms_status_to_squash_status (status);
}

static SquashStatus
squash_ms_decompress_buffer (SquashCodec* codec,
                             size_t* decompressed_length,
                             uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                             size_t compressed_length,
                             const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                             SquashOptions* options) {
  MSCompStatus status = ms_decompress (squash_ms_format_from_codec (codec),
                                       compressed, compressed_length, decompressed, decompressed_length);
  return squash_ms_status_to_squash_status (status);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lznt1", name) == 0 ||
      strcmp ("xpress", name) == 0 ||
      strcmp ("xpress-huffman", name) == 0) {
    funcs->get_max_compressed_size = squash_ms_get_max_compressed_size;
    funcs->decompress_buffer       = squash_ms_decompress_buffer;
    funcs->compress_buffer         = squash_ms_compress_buffer;
    if (strcmp ("lznt1", name) == 0) {
      funcs->info                    = SQUASH_CODEC_INFO_CAN_FLUSH;
      funcs->create_stream           = squash_ms_create_stream;
      funcs->process_stream          = squash_ms_process_stream;
    }
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
