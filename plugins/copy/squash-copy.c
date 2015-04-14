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

#include <squash/squash.h>

typedef struct SquashCopyStream_s {
  SquashStream base_object;
} SquashCopyStream;

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec    (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_copy_stream_init     (SquashCopyStream* stream,
                                                      SquashCodec* codec,
                                                      SquashStreamType stream_type,
                                                      SquashOptions* options,
                                                      SquashDestroyNotify destroy_notify);
static SquashCopyStream* squash_copy_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void              squash_copy_stream_destroy  (void* stream);
static void              squash_copy_stream_free     (void* stream);


static size_t
squash_copy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length;
}

static size_t
squash_copy_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  return compressed_length;
}

static SquashCopyStream*
squash_copy_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashCopyStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashCopyStream*) malloc (sizeof (SquashCopyStream));
  squash_copy_stream_init (stream, codec, stream_type, options, squash_copy_stream_free);

  return stream;
}

static void
squash_copy_stream_init (SquashCopyStream* stream,
                          SquashCodec* codec,
                          SquashStreamType stream_type,
                          SquashOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);
}

static void
squash_copy_stream_destroy (void* stream) {
  squash_stream_destroy (stream);
}

static void
squash_copy_stream_free (void* stream) {
  squash_copy_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_copy_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_copy_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_copy_process_stream (SquashStream* stream, SquashOperation operation) {
  const size_t cp_size = stream->avail_in < stream->avail_out ? stream->avail_in : stream->avail_out;

  if (cp_size != 0) {
    memcpy (stream->next_out, stream->next_in, cp_size);
    stream->avail_in -= cp_size;
    stream->next_in += cp_size;
    stream->avail_out -= cp_size;
    stream->next_out += cp_size;
  }

  return (stream->avail_in != 0) ? SQUASH_PROCESSING : SQUASH_OK;
}

static SquashStatus
squash_copy_compress_buffer (SquashCodec* codec,
                             uint8_t* compressed, size_t* compressed_length,
                             const uint8_t* uncompressed, size_t uncompressed_length,
                             SquashOptions* options) {
  if (*compressed_length < uncompressed_length)
    return squash_error (SQUASH_BUFFER_FULL);

  memcpy (compressed, uncompressed, uncompressed_length);
  *compressed_length = uncompressed_length;

  return SQUASH_OK;
}

static SquashStatus
squash_copy_decompress_buffer (SquashCodec* codec,
                               uint8_t* decompressed, size_t* decompressed_length,
                               const uint8_t* compressed, size_t compressed_length,
                               SquashOptions* options) {
  if (*decompressed_length < compressed_length)
    return squash_error (SQUASH_BUFFER_FULL);

  memcpy (decompressed, compressed, compressed_length);
  *decompressed_length = compressed_length;

  return SQUASH_OK;
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("copy", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    funcs->get_uncompressed_size = squash_copy_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_copy_get_max_compressed_size;
    funcs->decompress_buffer = squash_copy_decompress_buffer;
    funcs->compress_buffer = squash_copy_compress_buffer;
    funcs->create_stream = squash_copy_create_stream;
    funcs->process_stream = squash_copy_process_stream;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
