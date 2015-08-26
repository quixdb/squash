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

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>

#if !defined(_WIN32)
#include <sys/mman.h>
#endif

#include "internal.h"
#include "tinycthread/source/tinycthread.h"

#ifndef SQUASH_CODEC_FILE_BUF_SIZE
#  define SQUASH_CODEC_FILE_BUF_SIZE ((size_t) (1024 * 1024))
#endif

/**
 * @struct _SquashCodecImpl
 * @brief Function table for plugins
 *
 * This struct should only be used from within a plugin.
 *
 * This structure may grow over time to accomodate new features, so
 * when setting up the callbacks in a plugin you must set each field
 * individually instead of copying an entire instance of the struct.
 */

/**
 * @var _SquashCodecImpl::info
 * @brief Capability information about the codec
 */

/**
 * @var _SquashCodecImpl::options
 * @brief options which may bo passed to the codec to modify its operation
 */

/**
 * @var _SquashCodecImpl::create_stream
 * @brief Create a new %SquashStream.
 *
 * @param codec The codec.
 * @param stream_type The type of stream to create.
 * @param options The stream options.
 * @return A new %SquashStream.
 *
 * @see squash_codec_create_stream
 */

/**
 * @var _SquashCodecImpl::process_stream
 * @brief Process a %SquashStream.
 *
 * @param stream The stream.
 * @param operation The operation to perform.
 * @return A status code.
 *
 * @see squash_stream_process
 */

/**
 * @var _SquashCodecImpl::get_uncompressed_size
 * @brief Get the buffer's uncompressed size.
 *
 * @param codec The codec.
 * @param compressed Compressed data.
 * @param compressed_length Compressed data length (in bytes).
 * @return Size of the uncompressed data, or 0 if unknown.
 *
 * @see squash_codec_get_uncompressed_size
 */

/**
 * @var _SquashCodecImpl::get_max_compressed_size
 * @brief Get the maximum compressed size.
 *
 * @param codec The codec.
 * @param uncompressed_length Size of the uncompressed data.
 * @returns The maximum buffer size necessary to contain the
 *   compressed data.
 *
 * @see squash_codec_get_max_compressed_size
 */

/**
 * @var _SquashCodecImpl::decompress_buffer
 * @brief Decompress a buffer.
 *
 * @param codec The codec.
 * @param compressed The compressed data.
 * @param compressed_length The length of the compressed data.
 * @param uncompressed Buffer in which to store the uncompressed data.
 * @param uncompressed_length Location of the buffer size on input,
 *   used to store the length of the uncompressed data on output.
 * @param options Decompression options (or *NULL*)
 *
 * @see squash_codec_decompress_with_options
 */

/**
 * @var _SquashCodecImpl::compress_buffer
 * @brief Compress a buffer.
 *
 * @param codec The codec.
 * @param uncompressed The uncompressed data.
 * @param uncompressed_length The length of the uncompressed data.
 * @param compressed Buffer in which to store the compressed data.
 * @param compressed_length Location of the buffer size on input,
 *   used to store the length of the compressed data on output.
 * @param options Compression options (or *NULL*)
 *
 * @see squash_codec_compress_with_options
 */

/**
 * @var _SquashCodecImpl::compress_buffer_unsafe
 * @brief Compress a buffer.
 *
 * Plugins implementing this function can be sure that @a compressed
 * is at least as long as the maximum compressed length for a buffer
 * of @a uncompressed_length bytes.
 *
 * @param codec The codec.
 * @param uncompressed The uncompressed data.
 * @param uncompressed_length The length of the uncompressed data.
 * @param compressed Buffer in which to store the compressed data.
 * @param compressed_length Location of the buffer size on input,
 *   used to store the length of the compressed data on output.
 * @param options Compression options (or *NULL*)
 *
 * @see squash_codec_compress_with_options
 */

/**
 * @var _SquashCodecImpl::_reserved1
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved2
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved3
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved4
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved5
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved6
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved7
 * @brief Reserved for future use.
 */

/**
 * @var _SquashCodecImpl::_reserved8
 * @brief Reserved for future use.
 */

int
squash_codec_compare (SquashCodec* a, SquashCodec* b) {
  assert (a != NULL);
  assert (b != NULL);

  return strcmp (a->name, b->name);
}

int
squash_codec_extension_compare (SquashCodec* a, SquashCodec* b) {
  assert (a != NULL);
  assert (b != NULL);

  return strcmp (a->extension, b->extension);
}

/**
 * @defgroup SquashCodec SquashCodec
 * @brief A compression/decompression codec
 *
 * @{
 */

/**
 * @typedef SquashCodecForeachFunc
 * @brief Squashlback to be invoked on each @ref SquashCodec in a set
 *
 * @param codec A codec
 * @param data User-supplied data
 */

/**
 * @struct _SquashCodec
 * @brief A compression/decompression codec
 */

/**
 * @enum SquashCodecInfo
 * @brief Information about the codec
 *
 * This is a bitmask describing characteristics and features of the
 * codec.
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_CAN_FLUSH
 * @brief Flushing is supported
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_RUN_IN_THREAD
 * @brief The data is processed in a background thread
 *
 * This is an implementation detail for Squash—it should not be
 * important for anyone but plugin authors or Squash maintainers.
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_DECOMPRESS_SAFE
 * @brief The codec will not write outside of the buffer supplied to
 *   it during decompression.
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_AUTO_MASK
 * @brief Mask of flags which are automatically set based on which
 *   callbacks are provided.
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_VALID
 * @brief The codec is valid
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE
 * @brief The compressed data encodes the length of the uncompressed
 *   data without having to decompress it.
 */

/**
 * @var SquashCodecInfo::SQUASH_CODEC_INFO_NATIVE_STREAMING
 * @brief The codec natively supports a streaming interface.
 */

/**
 * @def SQUASH_CODEC_INFO_INVALID
 * @brief Invalid codec
 */

/**
 * @brief Get the name of a %SquashCodec
 *
 * @param codec The codec
 * @return The codec's name
 */
const char*
squash_codec_get_name (SquashCodec* codec) {
  assert (codec != NULL);

  return codec->name;
}

/**
 * @brief Get the priority of a %SquashCodec
 *
 * @param codec The codec
 * @return The codec's priority
 */
unsigned int
squash_codec_get_priority (SquashCodec* codec) {
  assert (codec != NULL);

  return codec->priority;
}

/**
 * @brief Get the plugin associated with a codec
 *
 * @param codec The codec
 * @return The plugin to which the codec belongs
 */
SquashPlugin*
squash_codec_get_plugin (SquashCodec* codec) {
  assert (codec != NULL);

  return codec->plugin;
}

/**
 * @brief Initialize a codec
 *
 * @note This function is generally only useful inside of a callback
 * passed to ::squash_foreach_codec or ::squash_plugin_foreach_codec.  Every
 * other way to get a codec (such as ::squash_get_codec or
 * ::squash_plugin_get_codec) will initialize the codec as well (and
 * return *NULL* instead of the codec if initialization fails).  The
 * foreach functions, however, do not initialize the codec since
 * doing so requires actually loading the plugin.
 *
 * @param codec The codec.
 * @return A status code.
 * @retval SQUASH_OK Codec successfully initialized.
 * @retval SQUASH_UNABLE_TO_LOAD Failed to load the codec.
 */
SquashStatus
squash_codec_init (SquashCodec* codec) {
  return squash_plugin_init_codec (codec->plugin, codec, &(codec->impl));
}

/**
 * @brief Get the codec's function table
 *
 * @param codec The codec.
 * @return The function table.
 */
SquashCodecImpl*
squash_codec_get_impl (SquashCodec* codec) {
  if (codec->initialized != 1) {
    SquashStatus res = squash_plugin_init_codec (codec->plugin, codec, &(codec->impl));
    if (res != SQUASH_OK) {
      return NULL;
    }
  }

  return &(codec->impl);
}

/**
 * @brief Get the uncompressed size of the compressed buffer
 *
 * This function is only useful for codecs with the @ref
 * SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE flag set.  For situations
 * where the codec does not know the uncompressed size, *0* will be
 * returned.
 *
 * @param codec The codec
 * @param compressed The compressed data
 * @param compressed_length The length of the compressed data
 * @return The uncompressed size, or *0* if unknown
 */
size_t
squash_codec_get_uncompressed_size (SquashCodec* codec,
                                    size_t compressed_length,
                                    const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  SquashCodecImpl* impl = NULL;

  assert (codec != NULL);

  impl = squash_codec_get_impl (codec);
  if (impl != NULL && impl->get_uncompressed_size != NULL) {
    return impl->get_uncompressed_size (codec, compressed_length, compressed);
  } else {
    return 0;
  }
}

/**
 * @brief Get the maximum buffer size necessary to store compressed data.
 *
 * Typically the return value will be some percentage larger than the
 * uncompressed length, plus a few bytes.  For example, for bzip2 it
 * is the uncompressed length plus 1%, plus an additional 600 bytes.
 *
 * @warning The result of this function is not guaranteed to be
 * correct for use with the @ref SquashStream API—it should only be
 * used with the single-squashl buffer-to-buffer functions such as
 * ::squash_codec_compress and ::squash_codec_compress_with_options.
 *
 * @param codec The codec
 * @param uncompressed_length Size of the uncompressed data in bytes
 * @return The maximum size required to store a compressed buffer
 *   representing @a uncompressed_length of uncompressed data.
 */
size_t
squash_codec_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  SquashCodecImpl* impl = NULL;

  assert (codec != NULL);

  impl = squash_codec_get_impl (codec);
  if (impl != NULL && impl->get_max_compressed_size != NULL) {
    return impl->get_max_compressed_size (codec, uncompressed_length);
  } else {
    return 0;
  }
}

/**
 * @brief Get the maximum buffer size necessary to store compressed data.
 *
 * Typically the return value will be some percentage larger than the
 * uncompressed length, plus a few bytes.  For example, for bzip2 it
 * is the uncompressed length plus 1%, plus an additional 600 bytes.
 *
 * @warning The result of this function is not guaranteed to be
 * correct for use with the @ref SquashStream API—it should only be
 * used with the single-squashl buffer-to-buffer functions such as
 * ::squash_codec_compress and ::squash_codec_compress_with_options.
 *
 * @param codec The name of the codec
 * @param uncompressed_length Size of the uncompressed data in bytes
 * @return The maximum size required to store a compressed buffer
 *   representing @a uncompressed_length of uncompressed data.
 * @see squash_codec_get_max_compressed_size
 */
size_t
squash_get_max_compressed_size (const char* codec, size_t uncompressed_length) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL) {
    return 0;
  }

  return squash_codec_get_max_compressed_size (codec_real, uncompressed_length);
}

/**
 * @brief Create a new stream with existing @ref SquashOptions
 *
 * @param codec The codec
 * @param stream_type The direction of the stream
 * @param options The options for the stream, or *NULL* to use the
 *     defaults
 * @return A new stream, or *NULL* on failure
 */
SquashStream*
squash_codec_create_stream_with_options (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashCodecImpl* impl = NULL;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  impl = squash_codec_get_impl (codec);
  if (impl == NULL) {
    return NULL;
  }

  if (impl->create_stream != NULL) {
    return impl->create_stream (codec, stream_type, options);
  } else {
    if (impl->process_stream == NULL) {
      return (SquashStream*) squash_buffer_stream_new (codec, stream_type, options);
    } else {
      return NULL;
    }
  }
}

/**
 * @brief Create a new stream with existing @ref SquashOptions
 *
 * @param codec The codec
 * @param stream_type The direction of the stream
 * @param ... A variadic list of key/value option pairs, followed by
 *   *NULL*
 * @return A new stream, or *NULL* on failure
 */
SquashStream*
squash_codec_create_stream (SquashCodec* codec, SquashStreamType stream_type, ...) {
  va_list ap;
  SquashOptions* options;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  va_start (ap, stream_type);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_codec_create_stream_with_options (codec, stream_type, options);
}

/**
 * @brief Compress a buffer with an existing @ref SquashOptions
 *
 * @param codec The codec to use
 * @param[out] compressed Location to store the compressed data
 * @param[in,out] compressed_length Location storing the size of the
 *   @a compressed buffer on input, replaced with the actual size of
 *   the compressed data
 * @param uncompressed The uncompressed data
 * @param uncompressed_length Length of the uncompressed data (in bytes)
 * @param options Compression options
 * @return A status code
 */
SquashStatus
squash_codec_compress_with_options (SquashCodec* codec,
                                    size_t* compressed_length,
                                    uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                    size_t uncompressed_length,
                                    const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                    SquashOptions* options) {
  SquashCodecImpl* impl = NULL;

  assert (codec != NULL);

  assert (compressed != NULL);
  assert (uncompressed != NULL);

  impl = squash_codec_get_impl (codec);
  if (impl == NULL)
    return squash_error (SQUASH_UNABLE_TO_LOAD);

  if (compressed == uncompressed)
    return squash_error (SQUASH_INVALID_BUFFER);

  if (impl->compress_buffer ||
      impl->compress_buffer_unsafe) {
    size_t max_compressed_length = squash_codec_get_max_compressed_size (codec, uncompressed_length);

    if (*compressed_length >= max_compressed_length) {
      if (impl->compress_buffer_unsafe != NULL) {
        return impl->compress_buffer_unsafe (codec,
                                              compressed_length, compressed,
                                              uncompressed_length, uncompressed,
                                              options);
      } else {
        return impl->compress_buffer (codec,
                                       compressed_length, compressed,
                                       uncompressed_length, uncompressed,
                                       options);
      }
    } else if (impl->compress_buffer != NULL) {
      return impl->compress_buffer (codec,
                                     compressed_length, compressed,
                                     uncompressed_length, uncompressed,
                                     options);
    } else {
      SquashStatus status;
      uint8_t* tmp_buf = malloc (max_compressed_length);
      if (tmp_buf == NULL)
        return squash_error (SQUASH_MEMORY);

      status = impl->compress_buffer_unsafe (codec,
                                              &max_compressed_length, tmp_buf,
                                              uncompressed_length, uncompressed,
                                              options);
      if (status == SQUASH_OK) {
        if (*compressed_length < max_compressed_length) {
          *compressed_length = max_compressed_length;
          free (tmp_buf);
          return squash_error (SQUASH_BUFFER_FULL);
        } else {
          *compressed_length = max_compressed_length;
          memcpy (compressed, tmp_buf, max_compressed_length);
          free (tmp_buf);
          return SQUASH_OK;
        }
      } else {
        free (tmp_buf);
        return status;
      }
    }
  } else {
    SquashStatus status;
    SquashStream* stream;

    stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_COMPRESS, options);
    if (stream == NULL)
      return squash_error (SQUASH_FAILED);

    stream->next_in = uncompressed;
    stream->avail_in = uncompressed_length;
    stream->next_out = compressed;
    stream->avail_out = *compressed_length;

    do {
      status = squash_stream_process (stream);
    } while (status == SQUASH_PROCESSING);

    if (status != SQUASH_OK) {
      squash_object_unref (stream);
      return status;
    }

    do {
      status = squash_stream_finish (stream);
    } while (status == SQUASH_PROCESSING);

    if (status != SQUASH_OK) {
      squash_object_unref (stream);
      return status;
    }

    *compressed_length = stream->total_out;
    squash_object_unref (stream);
    return status;
  }
}

/**
 * @brief Compress a buffer
 *
 * @param codec The codec to use
 * @param[out] compressed Location to store the compressed data
 * @param[in,out] compressed_length Location storing the size of the
 *   @a compressed buffer on input, replaced with the actual size of
 *   the compressed data
 * @param uncompressed The uncompressed data
 * @param uncompressed_length Length of the uncompressed data (in bytes)
 * @param ... A variadic list of key/value option pairs, followed by
 *   *NULL*
 * @return A status code
 */
SquashStatus squash_codec_compress (SquashCodec* codec,
                                    size_t* compressed_length,
                                    uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                    size_t uncompressed_length,
                                    const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                    ...) {
  SquashOptions* options;
  va_list ap;

  assert (codec != NULL);

  va_start (ap, uncompressed);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_codec_compress_with_options (codec,
                                             compressed_length, compressed,
                                             uncompressed_length, uncompressed,
                                             options);
}

/**
 * @brief Decompress a buffer with an existing @ref SquashOptions
 *
 * @param codec The codec to use
 * @param[out] decompressed Location to store the decompressed data
 * @param[in,out] decompressed_length Location storing the size of the
 *   @a decompressed buffer on input, replaced with the actual size of
 *   the decompressed data
 * @param compressed The compressed data
 * @param compressed_length Length of the compressed data (in bytes)
 * @param options Compression options
 * @return A status code
 */
SquashStatus
squash_codec_decompress_with_options (SquashCodec* codec,
                                      size_t* decompressed_length,
                                      uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                      size_t compressed_length,
                                      const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                      SquashOptions* options) {
  SquashCodecImpl* impl = NULL;

  assert (codec != NULL);

  impl = squash_codec_get_impl (codec);
  if (impl == NULL)
    return squash_error (SQUASH_UNABLE_TO_LOAD);

  if (decompressed == compressed)
    return squash_error (SQUASH_INVALID_BUFFER);

  if (impl->decompress_buffer != NULL) {
    return impl->decompress_buffer (codec,
                                     decompressed_length, decompressed,
                                     compressed_length, compressed,
                                     options);
  } else {
    SquashStatus status;
    SquashStream* stream;

    stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_DECOMPRESS, options);
    stream->next_in = compressed;
    stream->avail_in = compressed_length;
    stream->next_out = decompressed;
    stream->avail_out = *decompressed_length;

    do {
      status = squash_stream_process (stream);
    } while (status == SQUASH_PROCESSING);

    if (status == SQUASH_END_OF_STREAM) {
      status = SQUASH_OK;
      *decompressed_length = stream->total_out;
    } else if (status == SQUASH_OK) {
      do {
        status = squash_stream_finish (stream);
      } while (status == SQUASH_PROCESSING);

      if (status == SQUASH_OK) {
        *decompressed_length = stream->total_out;
      }
    }

    assert (stream->stream_type == SQUASH_STREAM_DECOMPRESS);
    squash_object_unref (stream);

    return status;
  }
}

/**
 * @brief Decompress a buffer
 *
 * @param codec The codec to use
 * @param[out] decompressed The decompressed data
 * @param[in,out] decompressed_length Length of the decompressed data (in bytes)
 * @param compressed Location to store the compressed data
 * @param[in,out] compressed_length Location storing the size of the
 *   @a compressed buffer on input, replaced with the actual size of
 *   the compressed data
 * @param ... A variadic list of key/value option pairs, followed by
 *   *NULL*
 * @return A status code
 */
SquashStatus
squash_codec_decompress (SquashCodec* codec,
                         size_t* decompressed_length,
                         uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                         size_t compressed_length,
                         const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                         ...) {
  SquashOptions* options;
  va_list ap;
  SquashStatus res;

  assert (codec != NULL);

  va_start (ap, compressed);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  res = squash_codec_decompress_with_options (codec,
                                              decompressed_length, decompressed,
                                              compressed_length, compressed,
                                              options);

  return res;
}

/**
 * @brief Compress a buffer
 *
 * @param codec The name of the codec to use
 * @param[out] compressed Location to store the compressed data
 * @param[in,out] compressed_length Location storing the size of the
 *   @a compressed buffer on input, replaced with the actual size of
 *   the compressed data
 * @param uncompressed The uncompressed data
 * @param uncompressed_length Length of the uncompressed data (in bytes)
 * @param ... A variadic list of key/value option pairs, followed by
 *   *NULL*
 * @return A status code
 */
SquashStatus
squash_compress (const char* codec,
                 size_t* compressed_length,
                 uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                 size_t uncompressed_length,
                 const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                 ...) {
  SquashOptions* options;
  va_list ap;
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  va_start (ap, uncompressed);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_codec_compress_with_options (codec_real,
                                             compressed_length, compressed,
                                             uncompressed_length, uncompressed,
                                             options);
}

/**
 * @brief Compress a buffer with an existing @ref SquashOptions
 *
 * @param codec The name of the codec to use
 * @param[out] compressed Location to store the compressed data
 * @param[in,out] compressed_length Location storing the size of the
 *   @a compressed buffer on input, replaced with the actual size of
 *   the compressed data
 * @param uncompressed The uncompressed data
 * @param uncompressed_length Length of the uncompressed data (in bytes)
 * @param options Compression options, or *NULL* to use the defaults
 * @return A status code
 */
SquashStatus
squash_compress_with_options (const char* codec,
                              size_t* compressed_length,
                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                              size_t uncompressed_length,
                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                              SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  return squash_codec_compress_with_options (codec_real,
                                             compressed_length, compressed,
                                             uncompressed_length, uncompressed,
                                             options);
}

/**
 * @brief Decompress a buffer with an existing @ref SquashOptions
 *
 * @param codec The name of the codec to use
 * @param[out] decompressed Location to store the decompressed data
 * @param[in,out] decompressed_length Location storing the size of the
 *   @a decompressed buffer on input, replaced with the actual size of
 *   the decompressed data
 * @param compressed The compressed data
 * @param compressed_length Length of the compressed data (in bytes)
 * @param ... A variadic list of key/value option pairs, followed by
 *   *NULL*
 * @return A status code
 */
SquashStatus
squash_decompress (const char* codec,
                   size_t* decompressed_length,
                   uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                   size_t compressed_length,
                   const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                   ...) {
  SquashOptions* options;
  va_list ap;
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  va_start (ap, compressed);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_codec_decompress_with_options (codec_real,
                                            decompressed_length, decompressed,
                                            compressed_length, compressed,
                                            options);
}

/**
 * @brief Decompress a buffer
 *
 * @param codec The name of the codec to use
 * @param[out] decompressed Location to store the decompressed data
 * @param[in,out] decompressed_length Location storing the size of the
 *   @a decompressed buffer on input, replaced with the actual size of
 *   the decompressed data
 * @param compressed The compressed data
 * @param compressed_length Length of the compressed data (in bytes)
 * @param options Decompression options, or *NULL* to use the defaults
 * @return A status code
 */
SquashStatus squash_decompress_with_options (const char* codec,
                                             size_t* decompressed_length,
                                             uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                             size_t compressed_length,
                                             const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                             SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  return squash_codec_decompress_with_options (codec_real,
                                               decompressed_length, decompressed,
                                               compressed_length, compressed,
                                               options);
}

/**
 * @brief Create a new codec
 * @private
 *
 * @param plugin Plugin to which this codec belongs
 * @param name Name of the codec
 */
SquashCodec*
squash_codec_new (SquashPlugin* plugin, const char* name) {
  SquashCodec* codecp = (SquashCodec*) malloc (sizeof (SquashCodec));
  SquashCodec codec = { 0, };

  codec.plugin = plugin;
  codec.name = strdup (name);
  codec.priority = 50;
  SQUASH_TREE_ENTRY_INIT(codec.tree);

  *codecp = codec;

  /* squash_plugin_add_codec (plugin, codecp); */

  return codecp;
}

/**
 * @brief Set the codec's extension
 * @private
 *
 * @param codec The codec
 * @param name Extension of the codec
 */
void
squash_codec_set_extension (SquashCodec* codec, const char* extension) {
  if (codec->extension != NULL)
    free (codec->extension);

  codec->extension = (extension != NULL) ? strdup (extension) : NULL;
}

/**
 * @brief Get the codec's extension
 *
 * @param codec The codec
 * @return The extension, or *NULL* if none is known
 */
const char*
squash_codec_get_extension (SquashCodec* codec) {
  return codec->extension;
}

/**
 * @brief Set the codec priority
 * @private
 *
 * @param codec The codec
 * @param priority Priority of the codec
 */
void
squash_codec_set_priority (SquashCodec* codec, unsigned int priority) {
  codec->priority = priority;
}

static SquashStatus
squash_codec_process_file_with_options (SquashCodec* codec,
                                        SquashStreamType stream_type,
                                        FILE* output,
                                        size_t output_length,
                                        FILE* input,
                                        SquashOptions* options) {
  SquashStatus res = SQUASH_FAILED;
  SquashStream* stream;

  if (codec->impl.process_stream == NULL) {
    /* Attempt to mmap the input and output.  This short circuits the
       whole SquashBufferStream hack when possible, which can be a
       huge performance boost (50-100% for several codecs). */
    SquashMappedFile* in_map;

    in_map = squash_mapped_file_new (input, false);
    if (in_map != NULL) {
      SquashMappedFile* out_map;
      size_t max_output_size;

      if (stream_type == SQUASH_STREAM_COMPRESS) {
        max_output_size = squash_codec_get_max_compressed_size (codec, in_map->data_length);
      } else if (codec->impl.get_uncompressed_size != NULL) {
        max_output_size = squash_codec_get_uncompressed_size (codec, in_map->data_length, in_map->data);
      } else if (output_length != 0) {
        max_output_size = output_length;
      } else {
        max_output_size = in_map->data_length - 1;
        max_output_size |= max_output_size >> 1;
        max_output_size |= max_output_size >> 2;
        max_output_size |= max_output_size >> 4;
        max_output_size |= max_output_size >> 8;
        max_output_size |= max_output_size >> 16;
        max_output_size++;

        max_output_size <<= 3;
      }

      out_map = squash_mapped_file_new_full(output, false, 0, max_output_size);
      if (out_map != NULL) {
        size_t output_size = out_map->data_length;

        if (stream_type == SQUASH_STREAM_COMPRESS) {
          output_size = out_map->data_length;
          res = squash_codec_compress_with_options (codec, &output_size, out_map->data, in_map->data_length, in_map->data, options);
        } else {
          do {
            output_size = max_output_size;
            res = squash_codec_decompress_with_options (codec, &output_size, out_map->data, in_map->data_length, in_map->data, options);
            max_output_size <<= 1;
          } while (SQUASH_UNLIKELY(res == SQUASH_BUFFER_FULL) && output_length == 0 && codec->impl.get_uncompressed_size == NULL);
        }

        squash_mapped_file_free (out_map);
        out_map = NULL;

        /* We know that the mmap was successful, so not checking
           fileno() should be safe. */
        if (ftruncate (fileno (output), (off_t) output_size) == -1)
          res = SQUASH_FAILED;
        if (fseek (output, output_size, SEEK_SET) == -1)
          res = SQUASH_FAILED;
      }

      if (res != SQUASH_OK) {
        size_t ipos = in_map->data_offset;
        squash_mapped_file_free (in_map);
        fseek (input, ipos, SEEK_SET);
      } else {
        squash_mapped_file_free (in_map);
      }
      in_map = NULL;
    }

    if (res == SQUASH_OK)
      return res;
  }

  stream = squash_codec_create_stream_with_options (codec, stream_type, options);
  if (stream == NULL) {
    return squash_error (SQUASH_FAILED);
  }

  uint8_t* inbuf = malloc (SQUASH_CODEC_FILE_BUF_SIZE);
  uint8_t* outbuf = malloc (SQUASH_CODEC_FILE_BUF_SIZE);

  if (inbuf == NULL || outbuf == NULL) {
    squash_object_unref (stream);
    free (inbuf);
    free (outbuf);
    return squash_error (SQUASH_MEMORY);
  }

  while ( !feof (input) ) {
    stream->next_in = inbuf;
    stream->avail_in = fread (inbuf, 1, SQUASH_CODEC_FILE_BUF_SIZE, input);

    do {
      stream->next_out = outbuf;
      stream->avail_out = SQUASH_CODEC_FILE_BUF_SIZE;

      res = squash_stream_process (stream);

      fwrite (outbuf, 1, stream->next_out - outbuf, output);
    } while ( res == SQUASH_PROCESSING );

    if ( res != SQUASH_OK ) {
      if ( res == SQUASH_END_OF_STREAM && stream_type == SQUASH_STREAM_DECOMPRESS ) {
        res = SQUASH_OK;
      }
      squash_object_unref (stream);
      free (inbuf);
      free (outbuf);
      return res;
    }
  }

  do {
    stream->next_out = outbuf;
    stream->avail_out = SQUASH_CODEC_FILE_BUF_SIZE;

    res = squash_stream_finish (stream);

    fwrite (outbuf, 1, stream->next_out - outbuf, output);
  } while ( res == SQUASH_PROCESSING );

  squash_object_unref (stream);
  free (inbuf);
  free (outbuf);

  return res;
}

/**
 * @brief Compress a standard I/O stream with options
 *
 * A convienence function which will compress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param compressed Stream to write compressed data to
 * @param uncompressed Stream to compress
 * @param options Options, or *NULL* to use the defaults
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_codec_compress_file_with_options (SquashCodec* codec,
                                         FILE* compressed, FILE* uncompressed,
                                         SquashOptions* options) {
  return squash_codec_process_file_with_options (codec, SQUASH_STREAM_COMPRESS, compressed, 0, uncompressed, options);
}

/**
 * @brief Decompress a standard I/O stream
 *
 * A convienence function which will decompress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param decompressed Stream to write decompressed data to
 * @param decompressed_length Length of the decompressed data, or 0 if
 *   unknown
 * @param compressed Stream to decompress
 * @param options Options, or *NULL* to use the defaults
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_codec_decompress_file_with_options (SquashCodec* codec,
                                           FILE* decompressed,
                                           size_t decompressed_length,
                                           FILE* compressed,
                                           SquashOptions* options) {
  return squash_codec_process_file_with_options (codec, SQUASH_STREAM_DECOMPRESS, decompressed, decompressed_length, compressed, options);
}

/**
 * @brief Compress a standard I/O stream
 *
 * A convienence function which will compress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param compressed Stream to write compressed data to
 * @param uncompressed Stream to compress
 * @param ... Variadic *NULL*-terminated list of key/value option
 *   pairs
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_codec_compress_file (SquashCodec* codec,
                            FILE* compressed,
                            FILE* uncompressed,
                            ...) {
  SquashOptions* options;
  va_list ap;

  va_start (ap, uncompressed);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_codec_compress_file_with_options (codec, compressed, uncompressed, options);
}

/**
 * @brief Decompress a standard I/O stream
 *
 * A convienence function which will decompress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * For codecs which only provide a buffer to buffer API natively and
 * don't encode the size of the uncompressed content in the compressed
 * data, providing the decompressed length can be important.  Without
 * it Squash will have to make a guess as to how much memory to
 * allocate, try to decompress and, if that fails, keep trying with
 * progressively larger buffers.  Currently it starts with the next
 * highest power of two after the compressed size left shifted 3 bits,
 * and left shifts by one additional bit and tries again as long as
 * the return value is @ref SQUASH_BUFFER_FULL.  For codecs which
 * provide a native streaming API the @a decompressed_length is not
 * used.
 *
 * @param codec The codec to use
 * @param decompressed Stream to write decompressed data to
 * @param decompressed_length The size of the decompressed data, or 0
 *   if not known
 * @param compressed Stream to decompress
 * @param ... Variadic *NULL*-terminated list of key/value option
 *   pairs
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_codec_decompress_file (SquashCodec* codec,
                              FILE* decompressed,
                              size_t decompressed_length,
                              FILE* compressed,
                              ...) {
  SquashOptions* options;
  va_list ap;

  va_start (ap, compressed);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_codec_decompress_file_with_options (codec, decompressed, decompressed_length, compressed, options);
}

/**
 * @brief Compress a standard I/O stream with options
 *
 * A convienence function which will compress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param compressed Stream to write compressed data to
 * @param uncompressed Stream to compress
 * @param options Options, or *NULL* to use the defaults
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_compress_file_with_options (const char* codec,
                                   FILE* compressed, FILE* uncompressed,
                                   SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  return squash_codec_compress_file_with_options (codec_real, compressed, uncompressed, options);
}

/**
 * @brief Decompress a standard I/O stream
 *
 * A convienence function which will decompress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param decompressed Stream to write decompressed data to
 * @param decompressed_length Length of the decompressed data, or 0 if
 *   unknown
 * @param compressed Stream to decompress
 * @param options Options, or *NULL* to use the defaults
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_decompress_file_with_options (const char* codec,
                                     FILE* decompressed,
                                     size_t decompressed_length,
                                     FILE* compressed,
                                     SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  return squash_codec_decompress_file_with_options (codec_real, decompressed, decompressed_length, compressed, options);
}

/**
 * @brief Compress a standard I/O stream
 *
 * A convienence function which will compress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param compressed Stream to write compressed data to
 * @param uncompressed Stream to compress
 * @param ... Variadic *NULL*-terminated list of key/value option
 *   pairs
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_compress_file (const char* codec,
                      FILE* compressed, FILE* uncompressed,
                      ...) {
  SquashCodec* codec_real = squash_get_codec (codec);
  SquashOptions* options;
  va_list ap;

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  va_start (ap, uncompressed);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_compress_file_with_options (codec, compressed, uncompressed, options);
}

/**
 * @brief Decompress a standard I/O stream
 *
 * A convienence function which will decompress the contents of one
 * standard I/O stream and write the result to another.  Neither
 * stream will be closed when this function is finished—you must call
 * fclose yourself.
 *
 * @param codec The codec to use
 * @param decompressed Stream to write decompressed data to
 * @param decompressed_length Length of the decompressed data, or 0 if
 *   unknown
 * @param compressed Stream to decompress
 * @param ... Variadic *NULL*-terminated list of key/value option
 *   pairs
 * @return ::SQUASH_OK on success, or a negative error code on failure
 */
SquashStatus
squash_decompress_file (const char* codec,
                        FILE* decompressed,
                        size_t decompressed_length,
                        FILE* compressed,
                        ...) {
  SquashCodec* codec_real = squash_get_codec (codec);
  SquashOptions* options;
  va_list ap;

  if (codec_real == NULL)
    return squash_error (SQUASH_NOT_FOUND);

  va_start (ap, compressed);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_decompress_file_with_options (codec, decompressed, decompressed_length, compressed, options);
}

/**
 * @brief Get a bitmask of information about the codec
 *
 * @param codec The codec
 * @return the codec info
 */
SquashCodecInfo
squash_codec_get_info (SquashCodec* codec) {
  return codec->impl.info;
}

/**
 * @brief Get a bitmask of information about the codec
 *
 * @param codec The codec
 * @return the codec info, or @ref SQUASH_CODEC_INFO_INVALID if there
 *   is no such codec
 */
SquashCodecInfo
squash_get_info (const char* codec) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real != NULL) {
    return squash_codec_get_info (codec_real);
  } else {
    squash_error (SQUASH_NOT_FOUND);
    return SQUASH_CODEC_INFO_INVALID;
  }
}

/**
 * @brief Get a list of options applicable to the codec
 *
 * @param codec The codec
 * @return a list of options, terminated by an option with a NULL name
 */
const SquashOptionInfo*
squash_codec_get_option_info (SquashCodec* codec) {
  SquashCodecImpl* impl = squash_codec_get_impl (codec);
  return impl->options;
}

/**
 * @brief Get a list of options applicable to the codec
 *
 * @param codec name of the codec
 * @return a list of options, terminated by an option with a NULL name
 */
const SquashOptionInfo*
squash_get_option_info (const char* codec) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real != NULL)
    return squash_codec_get_option_info (codec_real);
  else
    return NULL;
}

static const SquashOptionValue*
squash_codec_get_option_value_by_name (SquashCodec* codec,
                                       SquashOptions* options,
                                       const char* key,
                                       SquashOptionType* type) {
  size_t c_option;
  const SquashOptionInfo* info;

  assert (codec != NULL);
  assert (key != NULL);

  info = squash_codec_get_option_info (codec);
  for (c_option = 0 ; info[c_option].name != NULL ; c_option++) {
    if (strcasecmp (key, info[c_option].name) == 0) {
      if (type != NULL)
        *type = info[c_option].type;
      return (options != NULL) ?
        &(options->values[c_option]) :
        &(info[c_option].default_value);
    }
  }

  if (type != NULL)
    *type = SQUASH_OPTION_TYPE_NONE;
  return NULL;
}

/**
 * @brief Get the string value for an option
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a string the result
 * is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param key the name of the option to retrieve the value of
 * @return the value of the option
 */
const char*
squash_codec_get_option_string (SquashCodec* codec,
                                SquashOptions* options,
                                const char* key) {
  SquashOptionType type;
  const SquashOptionValue* value = squash_codec_get_option_value_by_name (codec, options, key, &type);
  switch ((int) type) {
    case SQUASH_OPTION_TYPE_STRING:
      return value->string_value;
    default:
      squash_assert_unreachable();
  }
}

/**
 * @brief Get the string value for an option by index
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a string the result
 * is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param index the index of the option to retrieve the value of
 * @return the value of the option
 */
const char*
squash_codec_get_option_string_index (SquashCodec* codec,
                                      SquashOptions* options,
                                      size_t index) {
  if (options != NULL)
    return options->values[index].string_value;
  else
    return codec->impl.options[index].default_value.string_value;
}

/**
 * @brief Get the boolean value for an option
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a boolean the result
 * is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param key the name of the option to retrieve the value of
 * @return the value of the option
 */
bool
squash_codec_get_option_bool (SquashCodec* codec,
                              SquashOptions* options,
                              const char* key) {
  SquashOptionType type;
  const SquashOptionValue* value = squash_codec_get_option_value_by_name (codec, options, key, &type);
  switch ((int) type) {
    case SQUASH_OPTION_TYPE_BOOL:
      return value->bool_value;
    default:
      squash_assert_unreachable();
  }
}

/**
 * @brief Get the boolean value for an option by index
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a boolean the result
 * is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param index the index of the option to retrieve the value of
 * @return the value of the option
 */
bool
squash_codec_get_option_bool_index (SquashCodec* codec,
                                    SquashOptions* options,
                                    size_t index) {
  if (options != NULL)
    return options->values[index].bool_value;
  else
    return codec->impl.options[index].default_value.bool_value;
}

/**
 * @brief Get the integer value for an option
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not an integer the
 * result is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param key the name of the option to retrieve the value of
 * @return the value of the option
 */
int
squash_codec_get_option_int (SquashCodec* codec,
                             SquashOptions* options,
                             const char* key) {
  SquashOptionType type;
  const SquashOptionValue* value = squash_codec_get_option_value_by_name (codec, options, key, &type);
  switch ((int) type) {
    case SQUASH_OPTION_TYPE_INT:
    case SQUASH_OPTION_TYPE_RANGE_INT:
    case SQUASH_OPTION_TYPE_ENUM_STRING:
      return value->int_value;
    default:
      squash_assert_unreachable();
  }
}

/**
 * @brief Get the integer value for an option by index
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not an integer the
 * result is undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param index the index of the option to retrieve the value of
 * @return the value of the option
 */
int
squash_codec_get_option_int_index (SquashCodec* codec,
                                   SquashOptions* options,
                                   size_t index) {
  if (options != NULL)
    return options->values[index].int_value;
  else
    return codec->impl.options[index].default_value.int_value;
}

/**
 * @brief Get the size value for an option
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a size the result is
 * undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param key the name of the option to retrieve the value of
 * @return the value of the option
 */
size_t
squash_codec_get_option_size (SquashCodec* codec,
                              SquashOptions* options,
                              const char* key) {
  SquashOptionType type;
  const SquashOptionValue* value = squash_codec_get_option_value_by_name (codec, options, key, &type);
  switch ((int) type) {
    case SQUASH_OPTION_TYPE_SIZE:
    case SQUASH_OPTION_TYPE_RANGE_SIZE:
      return value->size_value;
    default:
      squash_assert_unreachable();
  }
}

/**
 * @brief Get the size value for an option by index
 *
 * Note that this function will not perform a conversion—if you use it
 * to request the value of an option which is not a size the result is
 * undefined.
 *
 * @param codec the relevant codec
 * @param options the options instance to retrieve the value from
 * @param index the index of the option to retrieve the value of
 * @return the value of the option
 */
size_t
squash_codec_get_option_size_index (SquashCodec* codec,
                                    SquashOptions* options,
                                    size_t index) {
  if (options != NULL)
    return options->values[index].size_value;
  else
    return codec->impl.options[index].default_value.size_value;
}

/**
 * @}
 */
