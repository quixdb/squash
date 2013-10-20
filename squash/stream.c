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

#include "config.h"
#include "squash.h"
#include "internal.h"

/**
 * @var _SquashStream::base_object
 * @brief Base object.
 */

/**
 * @var _SquashStream::priv
 * @brief Private data.
 *
 * This is managed internally by Squash and should not be modified by
 * consumers or plugins.
 */

/**
 * @var _SquashStream::next_in
 * @brief The next input data to consume.
 */

/**
 * @var _SquashStream::avail_in
 * @brief Size (in bytes) of available input.
 */

/**
 * @var _SquashStream::total_in
 * @brief The total number of bytes input.
 *
 * This is managed internally by Squash and should not be modified by
 * consumers or plugins.
 */

/**
 * @var _SquashStream::next_out
 * @brief The buffer to write output to.
 */

/**
 * @var _SquashStream::avail_out
 * @brief Number of bytes available in the output buffer.
 */

/**
 * @var _SquashStream::total_out
 * @brief Total number of bytes output.
 *
 * This is managed internally by Squash and should not be modified by
 * consumers or plugins.
 */

/**
 * @var _SquashStream::codec
 * @brief Codec used for this stream.
 */

/**
 * @var _SquashStream::options
 * @brief Options used for this stream.
 */

/**
 * @var _SquashStream::stream_type
 * @brief Stream type.
 */

/**
 * @var _SquashStream::state
 * @brief State the stream is in.
 *
 * This is managed internally by Squash and should not be modified by
 * consumers or plugins.
 */

/**
 * @var _SquashStream::user_data
 * @brief User data
 *
 * Note that this is for consumers of the library, *not* for plugins.
 * It should be safe to use this from your application.
 */

/**
 * @var _SquashStream::destroy_user_data
 * @brief Squashlback to invoke on *user_data* when it is no longer
 *   necessary.
 */

/**
 * @defgroup SquashStream SquashStream
 * @brief Low-level compression and decompression streams.
 *
 * @{
 */

/**
 * @enum _SquashStreamType
 * @brief Stream type.
 */

/**
 * @var _SquashStreamType::SQUASH_STREAM_COMPRESS
 * @brief A compression stream.
 */

/**
 * @var _SquashStreamType::SQUASH_STREAM_DECOMPRESS
 * @brief A decompression stream.
 */

/**
 * @struct _SquashStream
 * @extends _SquashObject
 * @brief Compression/decompression streams.
 */

/**
 * @brief Initialize a stream.
 * @protected
 *
 * @param stream The stream to initialize.
 * @param codec The codec to use.
 * @param stream_type The stream type.
 * @param options The options.
 * @param destroy_notify Function to squashl to destroy the instance.
 *
 * @see squash_object_init
 */
void
squash_stream_init (void* stream,
                    SquashCodec* codec,
                    SquashStreamType stream_type,
                    SquashOptions* options,
                    SquashDestroyNotify destroy_notify) {
  SquashStream* s;

  assert (stream != NULL);

  s = (SquashStream*) stream;

  squash_object_init (stream, false, destroy_notify);

  s->priv = NULL;

  s->next_in = NULL;
  s->avail_in = 0;
  s->total_in = 0;

  s->next_out = NULL;
  s->avail_out = 0;
  s->total_out = 0;

  s->codec = codec;
  s->options = (options != NULL) ? squash_object_ref (options) : NULL;
  s->stream_type = stream_type;
  s->state = SQUASH_STREAM_STATE_IDLE;

  s->user_data = NULL;
  s->destroy_user_data = NULL;
}

/**
 * @brief Destroy a stream.
 * @protected
 *
 * @param stream The stream.
 *
 * @see squash_object_destroy
 */
void
squash_stream_destroy (void* stream) {
  SquashStream* s;

  assert (stream != NULL);

  s = (SquashStream*) stream;

  if (s->destroy_user_data != NULL && s->user_data != NULL) {
    s->destroy_user_data (s->user_data);
  }

  if (s->options != NULL) {
    s->options = squash_object_unref (s->options);
  }

  squash_object_destroy (stream);
}

/**
 * @brief Create a new stream.
 *
 * @param codec The name of the codec.
 * @param stream_type Stream type.
 * @param ... List of key/value option pairs, followed by *NULL*
 * @return A new stream, or *NULL* on failure.
 */
SquashStream*
squash_stream_new (const char* codec,
                   SquashStreamType stream_type,
                   ...) {
  va_list options_list;
  SquashStream* stream;

  va_start (options_list, stream_type);
  stream = squash_stream_newv (codec, stream_type, options_list);
  va_end (options_list);

  return stream;
}

/**
 * @brief Create a new stream with a variadic list of options.
 *
 * @param codec The name of the codec.
 * @param stream_type Stream type.
 * @param options List of key/value option pairs, followed by *NULL*
 * @return A new stream, or *NULL* on failure.
 */
SquashStream*
squash_stream_newv (const char* codec,
                    SquashStreamType stream_type,
                    va_list options) {
  SquashOptions* opts;
  SquashCodec* codec_real;

  codec_real = squash_get_codec (codec);
  if (codec_real == NULL) {
    return NULL;
  }

  opts = squash_options_newv (codec_real, options);
  if (opts == NULL) {
    return NULL;
  }

  return squash_codec_create_stream_with_options (codec_real, stream_type, opts);
}

/**
 * @brief Create a new stream with key/value option arrays.
 *
 * @param codec The name of the codec.
 * @param stream_type Stream type.
 * @param keys *NULL*-terminated array of option keys.
 * @param values Array of option values.
 * @return A new stream, or *NULL* on failure.
 */
SquashStream*
squash_stream_newa (const char* codec,
                    SquashStreamType stream_type,
                    const char* const* keys,
                    const char* const* values) {
  return NULL;
}

/**
 * @brief Create a new stream with options.
 *
 * @param codec The name of the codec.
 * @param stream_type Stream type.
 * @param options An option group.
 * @return A new stream, or *NULL* on failure.
 */
SquashStream*
squash_stream_new_with_options (const char* codec,
                                SquashStreamType stream_type,
                                SquashOptions* options) {
  SquashCodec* codec_real;

  assert (codec != NULL);

  codec_real = squash_get_codec (codec);

  return (codec_real != NULL) ?
    squash_codec_create_stream_with_options (codec_real, stream_type, options) : NULL;
}

/**
 * @brief Process a stream.
 *
 * This method will attempt to process data in a stream.  It should be
 * squashled repeatedly, adding data to the *avail_in* field and removing
 * data from the *avail_out* field as necessary.
 *
 * @param stream The stream.
 * @return A status code.
 * @retval SQUASH_OK All input successfully consumed.  Check the output
 *   buffer for data then proceed with new input.
 * @retval SQUASH_PROCESSING Progress was made, but not all input could
 *   be consumed.  Remove some data from the output buffer and run
 *   ::squash_stream_process again.
 * @retval SQUASH_END_OF_STREAM The end of stream was reached.  You
 *   shouldn't squashl ::squash_stream_process again.  *Decompression only*.
 */
SquashStatus
squash_stream_process (SquashStream* stream) {
  SquashCodec* codec;
  SquashCodecFuncs* funcs;
  SquashStatus res;

  assert (stream != NULL);

  switch (stream->state) {
    case SQUASH_STREAM_STATE_IDLE:
    case SQUASH_STREAM_STATE_RUNNING:
      break;
    default:
      return SQUASH_INVALID_OPERATION;
  }

  codec = stream->codec;
  assert (codec != NULL);

  if (stream->avail_out == 0) {
    return SQUASH_BUFFER_FULL;
  }

  funcs = squash_codec_get_funcs (codec);

  const size_t avail_in = stream->avail_in;
  const size_t avail_out = stream->avail_out;

  if (funcs->process_stream != NULL) {
    res = funcs->process_stream (stream);
  } else if (funcs->create_stream == NULL && funcs->flush_stream == NULL && funcs->finish_stream == NULL) {
    res = squash_buffer_stream_process ((SquashBufferStream*) stream);
  } else {
    res = SQUASH_INVALID_OPERATION;
  }

  stream->total_in += (avail_in - stream->avail_in);
  stream->total_out += (avail_out - stream->avail_out);

  switch (res) {
    case SQUASH_OK:
    case SQUASH_PROCESSING:
      stream->state = SQUASH_STREAM_STATE_RUNNING;
      break;
    case SQUASH_END_OF_STREAM:
      stream->state = SQUASH_STREAM_STATE_FINISHED;
      break;
  }

  return res;
}

/**
 * @brief Flush a stream.
 *
 * This method will attempt to process data in a stream.  It should be
 * squashled repeatedly, adding data to the *avail_in* field and removing
 * data from the *avail_out* field as necessary.
 *
 * @param stream The stream.
 * @return A status code.
 */
SquashStatus
squash_stream_flush (SquashStream* stream) {
  SquashCodec* codec;
  SquashCodecFuncs* funcs = NULL;
  SquashStatus res;

  assert (stream != NULL);

  switch (stream->state) {
    case SQUASH_STREAM_STATE_RUNNING:
    case SQUASH_STREAM_STATE_FLUSHING:
    case SQUASH_STREAM_STATE_IDLE:
      break;
    default:
      return SQUASH_INVALID_OPERATION;
  }

  if (stream->stream_type != SQUASH_STREAM_COMPRESS) {
    return SQUASH_INVALID_OPERATION;
  }

  codec = stream->codec;
  assert (codec != NULL);

  funcs = squash_codec_get_funcs (codec);

  if (funcs != NULL && funcs->flush_stream != NULL) {
    const size_t avail_in = stream->avail_in;
    const size_t avail_out = stream->avail_out;

    res = funcs->flush_stream (stream);

    stream->total_in += (avail_in - stream->avail_in);
    stream->total_out += (avail_out - stream->avail_out);
  } else {
    res = SQUASH_INVALID_OPERATION;
  }

  switch (res) {
    case SQUASH_OK:
      stream->state = SQUASH_STREAM_STATE_RUNNING;
      break;
    case SQUASH_PROCESSING:
      stream->state = SQUASH_STREAM_STATE_FLUSHING;
      break;
  }

  return res;
}

/**
 * @brief Finish writing to a stream.
 *
 * @param stream The stream.
 * @return A status code.
 */
SquashStatus
squash_stream_finish (SquashStream* stream) {
  SquashCodec* codec;
  SquashCodecFuncs* funcs;
  SquashStatus res;

  assert (stream != NULL);

  switch (stream->state) {
    case SQUASH_STREAM_STATE_RUNNING:
    case SQUASH_STREAM_STATE_FINISHING:
    case SQUASH_STREAM_STATE_IDLE:
      break;
    case SQUASH_STREAM_STATE_FINISHED:
      return SQUASH_OK;
    default:
      return SQUASH_INVALID_OPERATION;
  }

  codec = stream->codec;
  assert (codec != NULL);

  funcs = squash_codec_get_funcs (codec);
  assert (funcs != NULL);

  const size_t avail_in = stream->avail_in;
  const size_t avail_out = stream->avail_out;

  if (funcs->finish_stream != NULL) {
    res = funcs->finish_stream (stream);
  } else if (funcs->create_stream == NULL && funcs->process_stream == NULL && funcs->flush_stream == NULL) {
    res = squash_buffer_stream_finish ((SquashBufferStream*) stream);
  } else {
    assert (funcs->process_stream != NULL);
    res = funcs->process_stream (stream);
  }

  stream->total_in += (avail_in - stream->avail_in);
  stream->total_out += (avail_out - stream->avail_out);

  switch (res) {
    case SQUASH_END_OF_STREAM:
      res = SQUASH_OK;
    case SQUASH_OK:
      stream->state = SQUASH_STREAM_STATE_FINISHED;
      break;
    case SQUASH_PROCESSING:
      stream->state = SQUASH_STREAM_STATE_FINISHING;
      break;
  }

  return res;
}

/**
 * @}
 */
