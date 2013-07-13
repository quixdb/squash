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
#include <pthread.h>

#include <stdio.h>

#include "config.h"
#include "squash.h"
#include "internal.h"

/**
 * @struct _SquashCodecFuncs
 * @brief Function table for plugins
 *
 * This struct should only be used from within a plugin.
 */

/**
 * @var _SquashCodecFuncs::create_options
 * @brief Create a new %SquashOptions instance.
 *
 * @param codec The codec.
 * @return a new %SquashOptions instance, or *NULL* on failure.
 */

/**
 * @var _SquashCodecFuncs::parse_option
 * @brief Parse an option.
 *
 * @param options The options to instance parse into.
 * @param key The option key.
 * @param value The option value.
 * @return A status code (*SQUASH_OK* on success)
 *
 * @see squash_options_parse_option
 */

/**
 * @var _SquashCodecFuncs::create_stream
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
 * @var _SquashCodecFuncs::process_stream
 * @brief Process a %SquashStream.
 *
 * @param stream The stream.
 * @return A status code.
 *
 * @see squash_stream_process
 */

/**
 * @var _SquashCodecFuncs::flush_stream
 * @brief Flush a %SquashStream.
 *
 * @param stream The stream.
 * @return A status code.
 *
 * @see squash_stream_flush
 */

/**
 * @var _SquashCodecFuncs::finish_stream
 * @brief Finish writing to stream.
 *
 * @param stream The stream.
 * @return A status code.
 *
 * @see squash_stream_finish
 */

/**
 * @var _SquashCodecFuncs::get_uncompressed_size
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
 * @var _SquashCodecFuncs::get_max_compressed_size
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
 * @var _SquashCodecFuncs::decompress_buffer
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
 * @var _SquashCodecFuncs::compress_buffer
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

int
squash_codec_compare (SquashCodec* a, SquashCodec* b) {
  assert (a != NULL);
  assert (b != NULL);

  return strcmp (a->name, b->name);
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
  return squash_plugin_init_codec (codec->plugin, codec, &(codec->funcs));
}

/**
 * @brief Get the codec's function table
 *
 * @param codec The codec.
 * @return The function table.
 */
SquashCodecFuncs*
squash_codec_get_funcs (SquashCodec* codec) {
  if (codec->initialized != 1) {
    SquashStatus res = squash_plugin_init_codec (codec->plugin, codec, &(codec->funcs));
    if (res != SQUASH_OK) {
      return NULL;
    }
  }

  return &(codec->funcs);
}

/**
 * @brief Whether or not the codec knows the uncompressed size of
 *   compressed data
 *
 * Some codecs, such as Snappy, will embed the uncompressed size inside
 * of a compressed buffer, which allows you to know exactly how much
 * space to allocate in order to successfully uncompress the buffer.
 * This function will return *true* if this is one such codec, or
 * *false* if not.
 *
 * @param codec the codec
 * @return whether or not the codec knows the uncompressed size
 */
bool
squash_codec_get_knows_uncompressed_size (SquashCodec* codec) {
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  return (funcs != NULL && funcs->get_uncompressed_size);
}

/**
 * @brief Get the uncompressed size of the compressed buffer
 *
 * This function is only useful for codecs where
 * ::squash_codec_get_knows_uncompressed_size is *true*.  For situations
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
                                    const uint8_t* compressed, size_t compressed_length) {
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  if (funcs != NULL && funcs->get_uncompressed_size != NULL) {
    return funcs->get_uncompressed_size (codec, compressed, compressed_length);
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
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  if (funcs != NULL && funcs->get_max_compressed_size != NULL) {
    return funcs->get_max_compressed_size (codec, uncompressed_length);
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
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  if (funcs != NULL && funcs->create_stream != NULL) {
    return funcs->create_stream (codec, stream_type, options);
  } else {
    if (funcs->process_stream == NULL && funcs->flush_stream == NULL && funcs->finish_stream == NULL) {
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
  SquashPlugin* plugin;
  va_list ap;
  SquashOptions* options;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  plugin = codec->plugin;
  assert (plugin != NULL);

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
                                    uint8_t* compressed, size_t* compressed_length,
                                    const uint8_t* uncompressed, size_t uncompressed_length,
                                    SquashOptions* options) {
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  if (funcs == NULL) {
    return SQUASH_UNABLE_TO_LOAD;
  } else if (funcs->compress_buffer != NULL) {
    return funcs->compress_buffer (codec,
                                   compressed, compressed_length,
                                   uncompressed, uncompressed_length,
                                   options);
  } else {
    SquashStatus status;
    SquashStream* stream;

    stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_COMPRESS, options);
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
                                    uint8_t* compressed, size_t* compressed_length,
                                    const uint8_t* uncompressed, size_t uncompressed_length, ...) {
  SquashOptions* options;
  va_list ap;

  assert (codec != NULL);

  va_start (ap, uncompressed_length);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_codec_compress_with_options (codec,
                                             compressed, compressed_length,
                                             uncompressed, uncompressed_length,
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
                                      uint8_t* decompressed, size_t* decompressed_length,
                                      const uint8_t* compressed, size_t compressed_length,
                                      SquashOptions* options) {
  SquashPlugin* plugin = NULL;
  SquashCodecFuncs* funcs = NULL;

  assert (codec != NULL);

  plugin = codec->plugin;
  assert (plugin != NULL);

  funcs = squash_codec_get_funcs (codec);
  if (funcs == NULL) {
    return SQUASH_UNABLE_TO_LOAD;
  } else if (funcs->decompress_buffer != NULL) {
    return funcs->decompress_buffer (codec,
                                     decompressed, decompressed_length,
                                     compressed, compressed_length,
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

    switch (status) {
      case SQUASH_END_OF_STREAM:
        status = SQUASH_OK;
      case SQUASH_OK:
        *decompressed_length = stream->total_out;
      default:
        squash_object_unref (stream);
    }

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
                         uint8_t* decompressed, size_t* decompressed_length,
                         const uint8_t* compressed, size_t compressed_length, ...) {
  SquashOptions* options;
  va_list ap;
  SquashStatus res;

  assert (codec != NULL);

  va_start (ap, compressed_length);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  res = squash_codec_decompress_with_options (codec,
                                              decompressed, decompressed_length,
                                              compressed, compressed_length,
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
                 uint8_t* compressed, size_t* compressed_length,
                 const uint8_t* uncompressed, size_t uncompressed_length, ...) {
  SquashOptions* options;
  va_list ap;
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return SQUASH_NOT_FOUND;

  va_start (ap, uncompressed_length);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_codec_compress_with_options (codec_real,
                                             compressed, compressed_length,
                                             uncompressed, uncompressed_length,
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
                              uint8_t* compressed, size_t* compressed_length,
                              const uint8_t* uncompressed, size_t uncompressed_length,
                              SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return SQUASH_NOT_FOUND;

  return squash_codec_compress_with_options (codec_real,
                                             compressed, compressed_length,
                                             uncompressed, uncompressed_length,
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
                   uint8_t* decompressed, size_t* decompressed_length,
                   const uint8_t* compressed, size_t compressed_length, ...) {
  SquashOptions* options;
  va_list ap;
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return SQUASH_NOT_FOUND;

  va_start (ap, compressed_length);
  options = squash_options_newv (codec_real, ap);
  va_end (ap);

  return squash_codec_decompress_with_options (codec_real,
                                            decompressed, decompressed_length,
                                            compressed, compressed_length,
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
                                       uint8_t* decompressed, size_t* decompressed_length,
                                       const uint8_t* compressed, size_t compressed_length,
                                       SquashOptions* options) {
  SquashCodec* codec_real = squash_get_codec (codec);

  if (codec_real == NULL)
    return SQUASH_NOT_FOUND;

  return squash_codec_decompress_with_options (codec_real,
                                            decompressed, decompressed_length,
                                            compressed, compressed_length,
                                            options);
}

/**
 * @brief Create a new codec
 * @private
 *
 * @param name Name of the codec
 * @param priority Codec priority
 * @param plugin Plugin to which this codec belongs
 */
SquashCodec*
squash_codec_new (char* name, unsigned int priority, SquashPlugin* plugin) {
  SquashCodec* codecp = (SquashCodec*) malloc (sizeof (SquashCodec));
  SquashCodec codec = { 0, };

  codec.plugin = plugin;
  codec.name = name;
  codec.priority = priority;
  SQUASH_TREE_ENTRY_INIT(codec.tree);

  *codecp = codec;

  squash_plugin_add_codec (plugin, codecp);

  return codecp;
}

/**
 * @}
 */
