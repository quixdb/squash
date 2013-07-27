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
#include <string.h>

#include <stdio.h>

#include "config.h"
#include "squash.h"
#include "internal.h"

static void
squash_buffer_stream_init (void* stream,
                           SquashCodec* codec,
                           SquashStreamType stream_type,
                           SquashOptions* options,
                           SquashDestroyNotify destroy_notify) {
  SquashBufferStream* s = (SquashBufferStream*) stream;

  squash_stream_init (stream, codec, stream_type, options, destroy_notify);

  s->head = NULL;
  s->last = NULL;
  s->last_pos = 0;
  s->input = NULL;
  s->output = NULL;
}

static void
squash_buffer_stream_destroy (void* stream) {
  SquashBufferStream* s = (SquashBufferStream*) stream;

  squash_slist_foreach ((SquashSList*) s->head, (SquashSListForeachFunc) free);
  if (s->input != NULL) {
    squash_buffer_free (s->input);
  }
  if (s->output != NULL) {
    squash_buffer_free (s->output);
  }

  squash_stream_destroy (stream);
}

static void
squash_buffer_stream_free (void* options) {
  squash_buffer_stream_destroy (options);
  free (options);
}

SquashBufferStream*
squash_buffer_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashBufferStream* stream;

  stream = (SquashBufferStream*) malloc (sizeof (SquashBufferStream));
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

  while (stream->base_object.avail_in > 0) {
    if (stream->last_pos == SQUASH_BUFFER_STREAM_BUFFER_SIZE ||
        stream->last == NULL) {
      stream->last = SQUASH_SLIST_APPEND(stream->last, SquashBufferStreamSList);
      stream->last_pos = 0;

      if (stream->head == NULL) {
        stream->head = stream->last;
      }
    }

    const size_t copy_size = MIN(SQUASH_BUFFER_STREAM_BUFFER_SIZE - stream->last_pos, stream->base_object.avail_in);
    memcpy (stream->last->data + stream->last_pos, stream->base_object.next_in, copy_size);
    stream->base_object.next_in += copy_size;
    stream->base_object.avail_in -= copy_size;
    stream->last_pos += copy_size;
  }

  return SQUASH_OK;
}

static void
squash_buffer_stream_consolidate (SquashBufferStream* stream) {
  if (stream->input == NULL) {
    stream->input = squash_buffer_new (stream->base_object.total_in);
  }

  {
    SquashBufferStreamSList* next;
    SquashBufferStreamSList* list = stream->head;

    for ( next = NULL ; list != NULL ; list = next ) {
      next = (SquashBufferStreamSList*) list->base.next;
      squash_buffer_append (stream->input, list->data, (next != NULL) ? SQUASH_BUFFER_STREAM_BUFFER_SIZE : stream->last_pos);
      free (list);
    }
  }

  stream->head = NULL;
  stream->last = NULL;
  stream->last_pos = 0;
}

SquashStatus
squash_buffer_stream_finish (SquashBufferStream* stream) {
  SquashStatus res;

  if (stream->base_object.avail_out == 0) {
    return SQUASH_BUFFER_FULL;
  }

  if (stream->input == NULL) {
    squash_buffer_stream_process (stream);
    squash_buffer_stream_consolidate (stream);
  }

  if (stream->output == NULL) {
    if (stream->base_object.stream_type == SQUASH_STREAM_COMPRESS) {
      size_t compressed_size = squash_codec_get_max_compressed_size (stream->base_object.codec, stream->input->length);
      stream->output = squash_buffer_new (compressed_size);
      res = squash_codec_compress_with_options (stream->base_object.codec,
                                                stream->output->data, &compressed_size,
                                                stream->input->data, stream->input->length,
                                                stream->base_object.options);

      if (res != SQUASH_OK) {
        return res;
      }

      stream->output->length = compressed_size;
    } else if (stream->base_object.stream_type == SQUASH_STREAM_DECOMPRESS) {
      size_t decompressed_size;

      if (squash_codec_get_knows_uncompressed_size (stream->base_object.codec)) {
        decompressed_size = squash_codec_get_uncompressed_size (stream->base_object.codec, stream->input->data, stream->input->length);
        stream->output = squash_buffer_new (decompressed_size);
        squash_buffer_set_size (stream->output, decompressed_size);
        res = squash_codec_decompress_with_options (stream->base_object.codec,
                                                    stream->output->data, &decompressed_size,
                                                    stream->input->data, stream->input->length, NULL);
      } else {
        /* Yes, this is horrible.  If you're hitting this code you
           should really try to change your application to use the
           buffer API or use a container codec which does contain size
           information (e.g., zlib or gzip instead of deflate). */
        size_t decompressed_size = stream->input->length;

        /* Round decompressed_size up the the next highest power of
           two. */
        decompressed_size--;
        decompressed_size |= decompressed_size >> 1;
        decompressed_size |= decompressed_size >> 2;
        decompressed_size |= decompressed_size >> 4;
        decompressed_size |= decompressed_size >> 8;
        decompressed_size |= decompressed_size >> 16;
        decompressed_size++;

        /* One more power of two, should catch all but the most highly
           compressed data. */
        decompressed_size <<= 1;

        stream->output = squash_buffer_new (decompressed_size);

        res = SQUASH_BUFFER_FULL;
        while ( res == SQUASH_BUFFER_FULL ) {
          squash_buffer_set_size (stream->output, decompressed_size);
          res = squash_codec_decompress_with_options (stream->base_object.codec,
                                                      stream->output->data, &(stream->output->length),
                                                      stream->input->data, stream->input->length, NULL);
          decompressed_size <<= 1;
        }
      }
    } else {
      assert (false);
    }
  }

  size_t copy_size = MIN(stream->output->length - stream->base_object.total_out, stream->base_object.avail_out);
  memcpy (stream->base_object.next_out, stream->output->data + stream->base_object.total_out, copy_size);
  stream->base_object.next_out += copy_size;
  stream->base_object.avail_out -= copy_size;

  return ((stream->base_object.total_out + copy_size) == stream->output->length) ?
    SQUASH_OK :
    SQUASH_PROCESSING;
}
