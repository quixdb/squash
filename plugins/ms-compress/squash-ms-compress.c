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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <squash/squash.h>

#include "ms-compress/include/mscomp.h"
#include "ms-compress/include/xpress_huff.h"

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

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
    HEDLEY_UNREACHABLE();
}

static SquashStatus
squash_ms_status_to_squash_status (MSCompStatus status) {
  switch ((int) status) {
    case MSCOMP_OK:
      return SQUASH_OK;
    case MSCOMP_ERRNO:
      return squash_error (SQUASH_FAILED);
    case MSCOMP_ARG_ERROR:
      return squash_error (SQUASH_BAD_PARAM);
    case MSCOMP_DATA_ERROR:
      return squash_error (SQUASH_FAILED);
    case MSCOMP_MEM_ERROR:
      return squash_error (SQUASH_MEMORY);
    case MSCOMP_BUF_ERROR:
      return squash_error (SQUASH_BUFFER_FULL);
    default:
      return squash_error (SQUASH_FAILED);
  }
}

static bool
squash_ms_init_stream (SquashStream* stream, SquashStreamType stream_type, SquashOptions* options, void* priv) {
  mscomp_stream* s = priv;

  MSCompStatus status;
  MSCompFormat format = squash_ms_format_from_codec (stream->codec);
  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    status = ms_deflate_init (format, s);
  } else if (stream->stream_type == SQUASH_STREAM_DECOMPRESS) {
    status = ms_inflate_init (format, s);
  } else {
    HEDLEY_UNREACHABLE();
  }

  if (HEDLEY_UNLIKELY(status != MSCOMP_OK)) {
    squash_object_unref (stream);
    return (squash_error (squash_ms_status_to_squash_status (status)), NULL);
  }

  return stream;
}

static void
squash_ms_destroy_stream (SquashStream* stream, void* priv) {
  mscomp_stream* s = priv;

  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      ms_deflate_end(s);
      break;
    case SQUASH_STREAM_DECOMPRESS:
      ms_inflate_end(s);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
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
    case SQUASH_OPERATION_TERMINATE:
      HEDLEY_UNREACHABLE ();
  }
  HEDLEY_UNREACHABLE();
}

static SquashStatus
squash_ms_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  SquashStatus status = SQUASH_FAILED;
  MSCompStatus res;
  mscomp_stream* s = priv;

  s->in = stream->next_in;
  s->in_avail = stream->avail_in;
  s->out = stream->next_out;
  s->out_avail = stream->avail_out;

  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      res = ms_deflate(s, squash_ms_comp_flush_from_operation (operation));
      break;
    case SQUASH_STREAM_DECOMPRESS:
      res = ms_inflate(s);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }

  stream->next_in = s->in;
  stream->avail_in = s->in_avail;
  stream->next_out = s->out;
  stream->avail_out = s->out_avail;

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
        case SQUASH_OPERATION_TERMINATE:
          HEDLEY_UNREACHABLE ();
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
        case SQUASH_OPERATION_TERMINATE:
          HEDLEY_UNREACHABLE ();
      }
      break;
  }

  return status;
}

static size_t
squash_ms_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return ms_max_compressed_size (squash_ms_format_from_codec (codec), uncompressed_size);
}

static SquashStatus
squash_ms_compress_buffer (SquashCodec* codec,
                           size_t* compressed_size,
                           uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                           size_t uncompressed_size,
                           const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                           SquashOptions* options) {
  MSCompStatus status = ms_compress (squash_ms_format_from_codec (codec),
                                     uncompressed, uncompressed_size, compressed, compressed_size);
  return squash_ms_status_to_squash_status (status);
}

static SquashStatus
squash_ms_decompress_buffer (SquashCodec* codec,
                             size_t* decompressed_size,
                             uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                             size_t compressed_size,
                             const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                             SquashOptions* options) {
  MSCompStatus status = ms_decompress (squash_ms_format_from_codec (codec),
                                       compressed, compressed_size, decompressed, decompressed_size);
  return squash_ms_status_to_squash_status (status);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("lznt1", name) == 0 ||
                    strcmp ("xpress", name) == 0 ||
                    strcmp ("xpress-huffman", name) == 0)) {
    impl->get_max_compressed_size = squash_ms_get_max_compressed_size;
    impl->decompress_buffer       = squash_ms_decompress_buffer;
    impl->compress_buffer         = squash_ms_compress_buffer;
    if (strcmp ("lznt1", name) == 0) {
      impl->info           = SQUASH_CODEC_INFO_CAN_FLUSH;
      impl->priv_size      = sizeof(mscomp_stream);
      impl->init_stream    = squash_ms_init_stream;
      impl->destroy_stream = squash_ms_destroy_stream;
      impl->process_stream = squash_ms_process_stream;
    }
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
