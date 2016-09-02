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
 *   Evan Nemerson <evan@nemerson.com>
 *   Eugene Kliuchnikov <eustas.ru+squash@gmail.com>
 */

#include <assert.h>
#include <string.h>

#include <squash/squash.h>

#include <brotli/decode.h>
#include <brotli/encode.h>

enum SquashBrotliOptionIndex {
  SQUASH_BROTLI_OPT_LEVEL = 0,
  SQUASH_BROTLI_OPT_WINDOW_SIZE,
  SQUASH_BROTLI_OPT_BLOCK_SIZE,
  SQUASH_BROTLI_OPT_MODE
};

static SquashOptionInfo squash_brotli_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 11 },
    .default_value.int_value = BROTLI_DEFAULT_QUALITY },
  { "window-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 10,
      .max = 24 },
    .default_value.int_value = BROTLI_DEFAULT_WINDOW },
  { "block-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 16,
      .max = 24,
      .modulus = 0,
      .allow_zero = true},
    .default_value.int_value = 0 },
  { "mode",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "generic", BROTLI_MODE_GENERIC },
        { "text", BROTLI_MODE_TEXT },
        { "font", BROTLI_MODE_FONT },
        { NULL, 0 } } },
    .default_value.int_value = BROTLI_MODE_GENERIC },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

typedef union {
  BrotliEncoderState* encoder;
  BrotliState* decoder;
} SquashBrotliPrivate;

SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static void*
squash_brotli_malloc (void* opaque, size_t size) {
  return squash_malloc (size);
}

static void
squash_brotli_free (void* opaque, void* ptr) {
  squash_free (ptr);
}

static bool
squash_brotli_init_stream (SquashStream* stream,
                           SquashStreamType stream_type,
                           SquashOptions* options,
                           void* priv) {
  SquashBrotliPrivate* p = (SquashBrotliPrivate*) priv;

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    p->encoder = BrotliEncoderCreateInstance(squash_brotli_malloc, squash_brotli_free, NULL);
    if (HEDLEY_UNLIKELY(p->encoder == NULL))
      return false;

    BrotliEncoderSetParameter(p->encoder, BROTLI_PARAM_QUALITY, squash_options_get_int_at (options, stream->codec, SQUASH_BROTLI_OPT_LEVEL));
    BrotliEncoderSetParameter(p->encoder, BROTLI_PARAM_LGWIN, squash_options_get_int_at (options, stream->codec, SQUASH_BROTLI_OPT_WINDOW_SIZE));
    BrotliEncoderSetParameter(p->encoder, BROTLI_PARAM_LGBLOCK, squash_options_get_int_at (options, stream->codec, SQUASH_BROTLI_OPT_BLOCK_SIZE));
    BrotliEncoderSetParameter(p->encoder, BROTLI_PARAM_MODE, squash_options_get_int_at (options, stream->codec, SQUASH_BROTLI_OPT_MODE));
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    p->decoder = BrotliCreateState(squash_brotli_malloc, squash_brotli_free, NULL);
    if (HEDLEY_UNLIKELY(p->decoder == NULL))
      return false;
  } else {
    HEDLEY_UNREACHABLE();
  }

  return true;
}

static void
squash_brotli_destroy_stream (SquashStream* stream, void* priv) {
  SquashBrotliPrivate* p = (SquashBrotliPrivate*) priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    BrotliEncoderDestroyInstance(p->encoder);
  } else if (stream->stream_type == SQUASH_STREAM_DECOMPRESS) {
    BrotliDestroyState(p->decoder);
  } else {
    HEDLEY_UNREACHABLE();
  }

  squash_stream_destroy (stream);
}

static BrotliEncoderOperation
squash_brotli_encoder_operation_from_squash_operation (const SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return BROTLI_OPERATION_PROCESS;
    case SQUASH_OPERATION_FLUSH:
      return BROTLI_OPERATION_FLUSH;
    case SQUASH_OPERATION_FINISH:
      return BROTLI_OPERATION_FINISH;
    case SQUASH_OPERATION_TERMINATE:
    default:
      HEDLEY_UNREACHABLE ();
  }
}

static SquashStatus
squash_brotli_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  SquashBrotliPrivate* p = (SquashBrotliPrivate*) priv;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    const int be_ret =
      BrotliEncoderCompressStream(p->encoder,
                                  squash_brotli_encoder_operation_from_squash_operation(operation),
                                  &(stream->avail_in), &(stream->next_in),
                                  &(stream->avail_out), &(stream->next_out),
                                  NULL);

    if (HEDLEY_UNLIKELY(be_ret != 1))
      return squash_error (SQUASH_FAILED);
    else if (stream->avail_in != 0 || BrotliEncoderHasMoreOutput(p->encoder))
      return SQUASH_PROCESSING;
    else
      return SQUASH_OK;
  } else if (stream->stream_type == SQUASH_STREAM_DECOMPRESS) {
    const BrotliResult bd_ret =
      BrotliDecompressStream(&(stream->avail_in), &(stream->next_in),
                             &(stream->avail_out), &(stream->next_out),
                             NULL,
                             p->decoder);

    switch (bd_ret) {
      case BROTLI_RESULT_SUCCESS:
        return SQUASH_OK;
      case BROTLI_RESULT_NEEDS_MORE_INPUT:
        return SQUASH_OK;
      case BROTLI_RESULT_NEEDS_MORE_OUTPUT:
        return SQUASH_PROCESSING;
      case BROTLI_RESULT_ERROR:
        return SQUASH_FAILED;
    }

    if (HEDLEY_UNLIKELY(bd_ret != BROTLI_RESULT_SUCCESS))
      return squash_error (SQUASH_FAILED);
  } else {
    HEDLEY_UNREACHABLE ();
  }

  HEDLEY_UNREACHABLE ();
}

static size_t
squash_brotli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return BrotliEncoderMaxCompressedSize(uncompressed_size);
}

static SquashStatus
squash_brotli_compress_buffer (SquashCodec* codec,
                               size_t* compressed_size,
                               uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                               size_t uncompressed_size,
                               const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                               SquashOptions* options) {
  const int quality = squash_options_get_int_at (options, codec, SQUASH_BROTLI_OPT_LEVEL);
  const int lgwin = squash_options_get_int_at (options, codec, SQUASH_BROTLI_OPT_WINDOW_SIZE);
  const BrotliEncoderMode mode = (BrotliEncoderMode)
    squash_options_get_int_at (options, codec, SQUASH_BROTLI_OPT_MODE);

  const int res = BrotliEncoderCompress (quality, lgwin, mode, uncompressed_size, uncompressed, compressed_size, compressed);

  return HEDLEY_LIKELY(res == 1) ? SQUASH_OK : squash_error (SQUASH_BUFFER_FULL);
}

static SquashStatus
squash_brotli_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_size,
                                 uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                                 size_t compressed_size,
                                 const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                                 SquashOptions* options) {
  const BrotliResult res = BrotliDecompressBuffer(compressed_size, compressed, decompressed_size, decompressed);

  return HEDLEY_LIKELY(res == BROTLI_RESULT_SUCCESS) ? SQUASH_OK : squash_error (SQUASH_BUFFER_FULL);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("brotli", name) == 0)) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->priv_size = sizeof(SquashBrotliPrivate);
    impl->options = squash_brotli_options;
    impl->get_max_compressed_size = squash_brotli_get_max_compressed_size;
    impl->init_stream = squash_brotli_init_stream;
    impl->destroy_stream = squash_brotli_destroy_stream;
    impl->process_stream = squash_brotli_process_stream;
    impl->decompress_buffer = squash_brotli_decompress_buffer;
    impl->compress_buffer = squash_brotli_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
