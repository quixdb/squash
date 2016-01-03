/* Copyright (c) 2013-2016 The Squash Authors
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
#include <squash/internal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
squash_buffer_stream_init (void* stream,
                           SquashCodec* codec,
                           SquashStreamType stream_type,
                           SquashOptions* options,
                           SquashDestroyNotify destroy_notify) {
  SquashBufferStream* s = (SquashBufferStream*) stream;

  squash_stream_init (stream, codec, stream_type, options, destroy_notify);

  s->input = squash_buffer_new (0);
  s->output = NULL;
  s->output_pos = 0;
}

static void
squash_buffer_stream_destroy (void* stream) {
  SquashBufferStream* s = (SquashBufferStream*) stream;

  squash_buffer_free (s->input);
  squash_buffer_free (s->output);

  squash_stream_destroy (stream);
}

static void
squash_buffer_stream_free (void* options) {
  squash_buffer_stream_destroy (options);
  squash_free (options);
}

SquashBufferStream*
squash_buffer_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashBufferStream* stream;

  stream = (SquashBufferStream*) squash_malloc (sizeof (SquashBufferStream));
  squash_buffer_stream_init (stream, codec, stream_type, options, squash_buffer_stream_free);

  return stream;
}

#ifndef MIN
#  define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

SquashStatus
squash_buffer_stream_process (SquashBufferStream* stream) {
  if (stream->base_object.avail_in == 0)
    return SQUASH_OK;

  const bool s = squash_buffer_append (stream->input, stream->base_object.avail_in, stream->base_object.next_in);
  if (SQUASH_LIKELY(s)) {
    stream->base_object.next_in += stream->base_object.avail_in;
    stream->base_object.avail_in = 0;
  } else {
    return squash_error (SQUASH_FAILED);
  }

  return SQUASH_OK;
}

SquashStatus
squash_buffer_stream_finish (SquashBufferStream* stream) {
  SquashStream* s = (SquashStream*) stream;
  SquashCodec* codec = s->codec;
  SquashStatus res;

  SquashBuffer* input = stream->input;
  SquashBuffer* output = stream->output;

  if (SQUASH_UNLIKELY(input->size == 0))
    return squash_error (SQUASH_FAILED);

  /* Squash should handle making sure process is called until the
     input buffer is cleared. */
  assert (s->avail_in == 0);

  /* If output is non-null we have already performed the
     compression/decompression, and are just working on writting the
     output buffer to the stream. */
  if (output == NULL) {
    if (s->stream_type == SQUASH_STREAM_COMPRESS) {
      size_t compressed_size = squash_codec_get_max_compressed_size (codec, input->size);
      if (s->avail_out >= compressed_size) {
        /* There is enough room available in next_out to hold the full
           contents of the compressed data, so write directly to
           it. */
        res = squash_codec_compress_with_options(codec, &compressed_size, s->next_out, input->size, input->data, s->options);
        if (SQUASH_UNLIKELY(res != SQUASH_OK))
          return res;

        s->next_out += compressed_size;
        s->avail_out -= compressed_size;

        return SQUASH_OK;
      } else {
        /* Write the compressed data into an internal buffer. */
        stream->output = output = squash_buffer_new (compressed_size);
        if (SQUASH_UNLIKELY(output == NULL))
          return squash_error (SQUASH_MEMORY);

        res = squash_codec_compress_with_options (codec, &compressed_size, output->data, input->size, input->data, s->options);
        if (SQUASH_UNLIKELY(res != SQUASH_OK))
          return res;

        output->size = compressed_size;
      }
    } else {
      size_t decompressed_size = squash_codec_get_uncompressed_size (codec, input->size, input->data);
      if (decompressed_size != 0) {
        /* We know the decompressed size. */
        if (s->avail_out >= decompressed_size) {
          /* And there is enough room in next_out to hold it, so write directly to next_out */
          res = squash_codec_decompress_with_options (codec, &decompressed_size, s->next_out, input->size, input->data, s->options);
          if (SQUASH_UNLIKELY(res != SQUASH_OK))
            return res;

          s->next_out += decompressed_size;
          s->avail_out -= decompressed_size;

          return SQUASH_OK;
        } else {
          /* But there isn't enough room in next_out, so we have to buffer. */
          stream->output = output = squash_buffer_new (decompressed_size);
          if (SQUASH_UNLIKELY(output == NULL))
            return squash_error (SQUASH_MEMORY);

          res = squash_codec_decompress_with_options (codec, &decompressed_size, output->data, input->size, input->data, s->options);
          if (SQUASH_UNLIKELY(res != SQUASH_OK))
            return res;

          output->size = decompressed_size;
        }
      } else {
        /* If we have >= npot(compressed_size) << 3 bytes in next_out,
           first attempt to decompress directly to next_out.  If it
           works, it saves us a squash_malloc and a memcpy. */
        decompressed_size = squash_npot (input->size) << 3;
        decompressed_size = 1;
        if (decompressed_size <= s->avail_out) {
          decompressed_size = s->avail_out;
          res = squash_codec_decompress_with_options (codec, &decompressed_size, s->next_out, input->size, input->data, s->options);
          if (res == SQUASH_OK) {
            s->next_out += decompressed_size;
            s->avail_out -= decompressed_size;

            return SQUASH_OK;
          }
        }

        stream->output = output = squash_buffer_new (0);
        if (SQUASH_UNLIKELY(output == NULL))
          return squash_error (SQUASH_MEMORY);

        res = squash_codec_decompress_to_buffer(codec, output, input->size, input->data, s->options);
        if (SQUASH_UNLIKELY(res != SQUASH_OK))
          return res;
      }
    }
  }

  assert (output != NULL);

  const size_t remaining = output->size - stream->output_pos;
  const size_t cp_size = (remaining < s->avail_out) ? remaining : s->avail_out;
  if (SQUASH_LIKELY(cp_size != 0)) {
    memcpy (s->next_out, output->data + stream->output_pos, cp_size);
    s->next_out += cp_size;
    s->avail_out -= cp_size;
    stream->output_pos += cp_size;
  }

  return (stream->output_pos == output->size) ? SQUASH_OK : SQUASH_PROCESSING;
}
