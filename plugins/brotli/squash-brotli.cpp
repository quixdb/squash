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

#include <squash/squash.h>

#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"

typedef struct SquashBrotliOptions_s {
  SquashOptions base_object;

  brotli::BrotliParams::Mode mode;
  bool enable_transforms;
} SquashBrotliOptions;

#define SQUASH_BROTLI_MAX_BLOCK_SIZE (1 << 21)
#define SQUASH_BROTLI_MAX_OUT_SIZE (1 << 22)

typedef struct SquashBrotliStream_s {
  SquashStream base_object;

  BrotliInput in;
  BrotliOutput out;

  SquashOperation operation;

  union {
    brotli::BrotliCompressor* comp;
    BrotliState decomp;
  } ctx;
} SquashBrotliStream;

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus                squash_plugin_init_codec      (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                 squash_brotli_options_init    (SquashBrotliOptions* options,
                                                           SquashCodec* codec,
                                                           SquashDestroyNotify destroy_notify);
static SquashBrotliOptions* squash_brotli_options_new     (SquashCodec* codec);
static void                 squash_brotli_options_destroy (void* options);
static void                 squash_brotli_options_free    (void* options);

static void                 squash_brotli_stream_init     (SquashBrotliStream* stream,
                                                           SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashBrotliOptions* options,
                                                           SquashDestroyNotify destroy_notify);
static SquashBrotliStream*  squash_brotli_stream_new      (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashBrotliOptions* options);
static void                 squash_brotli_stream_destroy  (void* stream);
static void                 squash_brotli_stream_free     (void* stream);

static int
squash_brotli_reader (void* user_data, uint8_t* buf, size_t size) {
  SquashBrotliStream* stream = (SquashBrotliStream*) user_data;

  size_t remaining = size;
  while (remaining != 0) {
    const size_t cp_size = (stream->base_object.avail_in < remaining) ? stream->base_object.avail_in : remaining;

    memcpy (buf + (size - remaining), stream->base_object.next_in, cp_size);
    stream->base_object.next_in += cp_size;
    stream->base_object.avail_in -= cp_size;
    remaining -= cp_size;

    if (remaining != 0) {
      if (stream->operation == SQUASH_OPERATION_FINISH)
        break;

      stream->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_OK);
    }
  }

  return (size - remaining);
}

static int
squash_brotli_writer (void* user_data, const uint8_t* buf, size_t size) {
  SquashBrotliStream* stream = (SquashBrotliStream*) user_data;
  size_t remaining = size;
  while (remaining != 0) {
    const size_t cp_size = (stream->base_object.avail_out < remaining) ? stream->base_object.avail_out : remaining;

    memcpy (stream->base_object.next_out, buf + (size - remaining), cp_size);
    stream->base_object.next_out += cp_size;
    stream->base_object.avail_out -= cp_size;
    remaining -= cp_size;

    if (remaining != 0) {
      stream->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_PROCESSING);
    }
  }

  return (size - remaining);
}

static void
squash_brotli_options_init (SquashBrotliOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->mode = brotli::BrotliParams::MODE_TEXT;
  options->enable_transforms = false;
}

static SquashBrotliOptions*
squash_brotli_options_new (SquashCodec* codec) {
  SquashBrotliOptions* options;

  options = (SquashBrotliOptions*) malloc (sizeof (SquashBrotliOptions));
  squash_brotli_options_init (options, codec, squash_brotli_options_free);

  return options;
}

static void
squash_brotli_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_brotli_options_free (void* options) {
  squash_brotli_options_destroy ((SquashBrotliOptions*) options);
  free (options);
}

static SquashOptions*
squash_brotli_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_brotli_options_new (codec);
}

static SquashStatus
squash_brotli_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashBrotliOptions* opts = (SquashBrotliOptions*) options;

  assert (opts != NULL);

  if (strcasecmp (key, "mode") == 0) {
    if (strcasecmp (key, "text") == 0) {
      opts->mode = brotli::BrotliParams::MODE_TEXT;
    } else if (strcasecmp (key, "font")) {
      opts->mode = brotli::BrotliParams::MODE_FONT;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "enable-transforms") == 0) {
    if (strcasecmp (value, "true") == 0) {
      opts->enable_transforms = true;
    } else if (strcasecmp (value, "false")) {
      opts->enable_transforms = false;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static SquashBrotliStream*
squash_brotli_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashBrotliOptions* options) {
  SquashBrotliStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashBrotliStream*) malloc (sizeof (SquashBrotliStream));
  squash_brotli_stream_init (stream, codec, stream_type, options, squash_brotli_stream_free);

  return stream;
}

static void
squash_brotli_stream_init (SquashBrotliStream* s,
                           SquashCodec* codec,
                           SquashStreamType stream_type,
                           SquashBrotliOptions* options,
                           SquashDestroyNotify destroy_notify) {
  SquashStream* stream = (SquashStream*) s;

  s->in.cb_ = squash_brotli_reader;
  s->in.data_ = (void*) stream;
  s->out.cb_ = squash_brotli_writer;
  s->out.data_ = (void*) stream;

  squash_stream_init (stream, codec, stream_type, (SquashOptions*) options, destroy_notify);
}

static void
squash_brotli_stream_destroy (void* stream) {
  SquashBrotliStream* s = (SquashBrotliStream*) stream;

  squash_stream_destroy (stream);
}

static void
squash_brotli_stream_free (void* stream) {
  squash_brotli_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_brotli_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_brotli_stream_new (codec, stream_type, (SquashBrotliOptions*) options);
}

static SquashStatus
squash_brotli_compress_stream (SquashStream* stream, SquashOperation operation) {
  SquashStatus res = SQUASH_FAILED;
  SquashBrotliStream* s = (SquashBrotliStream*) stream;

  brotli::BrotliParams params;

  if (stream->options != NULL) {
    SquashBrotliOptions* opts = (SquashBrotliOptions*) stream->options;
    params.mode = opts->mode;
    params.enable_transforms = opts->enable_transforms;
  }

  s->ctx.comp = new brotli::BrotliCompressor (params);
  s->ctx.comp->WriteStreamHeader ();

  int block_size;
  uint8_t* input_buffer = (uint8_t*) malloc (SQUASH_BROTLI_MAX_BLOCK_SIZE);
  uint8_t* output_buffer = (uint8_t*) malloc (SQUASH_BROTLI_MAX_OUT_SIZE);
  size_t output_size;

  if (input_buffer == NULL || output_buffer == NULL) {
    res = squash_error (SQUASH_MEMORY);
    goto cleanup;
  }

  while ((block_size = BrotliRead (s->in, input_buffer, SQUASH_BROTLI_MAX_BLOCK_SIZE)) != 0) {
    output_size = SQUASH_BROTLI_MAX_OUT_SIZE;
    s->ctx.comp->WriteMetaBlock (block_size, input_buffer, false, &output_size, output_buffer);
    if (BrotliWrite (s->out, output_buffer, output_size) != output_size) {
      res = SQUASH_FAILED;
      goto cleanup;
    }
  }

  output_size = SQUASH_BROTLI_MAX_OUT_SIZE;
  s->ctx.comp->WriteMetaBlock (block_size, input_buffer, true, &output_size, output_buffer);
  if (BrotliWrite (s->out, output_buffer, output_size) != output_size) {
    res = squash_error (SQUASH_FAILED);
    goto cleanup;
  }

  res = SQUASH_OK;

 cleanup:
  free (input_buffer);
  free (output_buffer);
  delete s->ctx.comp;
  s->ctx.comp = NULL;

  return res;
}

static SquashStatus
squash_brotli_decompress_stream (SquashStream* stream, SquashOperation operation) {
  SquashBrotliStream* s = (SquashBrotliStream*) stream;

  if (!BrotliDecompress (s->in, s->out)) {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

static SquashStatus
squash_brotli_process_stream (SquashStream* stream, SquashOperation operation) {
  if (stream->stream_type == SQUASH_STREAM_COMPRESS)
    return squash_brotli_compress_stream (stream, operation);
  else
    return squash_brotli_decompress_stream (stream, operation);
}

static size_t
squash_brotli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + 4;
}

static SquashStatus
squash_brotli_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_length,
                                 uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                 size_t compressed_length,
                                 const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                 SquashOptions* options) {
  try {
    int res = BrotliDecompressBuffer (compressed_length, compressed,
                                      decompressed_length, decompressed);
    return (res == 1) ? SQUASH_OK : SQUASH_FAILED;
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }
}

static SquashStatus
squash_brotli_compress_buffer (SquashCodec* codec,
                               size_t* compressed_length,
                               uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                               size_t uncompressed_length,
                               const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                               SquashOptions* options) {
  brotli::BrotliParams params;

  if (options != NULL) {
    SquashBrotliOptions* opts = (SquashBrotliOptions*) options;
    params.mode = opts->mode;
    params.enable_transforms = opts->enable_transforms;
  }

  try {
    int res = brotli::BrotliCompressBuffer (params,
                                            uncompressed_length, uncompressed,
                                            compressed_length, compressed);
    return (res == 1) ? SQUASH_OK : SQUASH_FAILED;
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
  }
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("brotli", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    funcs->create_options = squash_brotli_create_options;
    funcs->parse_option = squash_brotli_parse_option;
    funcs->get_max_compressed_size = squash_brotli_get_max_compressed_size;
    funcs->create_options = squash_brotli_create_options;
    funcs->parse_option = squash_brotli_parse_option;
    funcs->create_stream = squash_brotli_create_stream;
    funcs->process_stream = squash_brotli_process_stream;
    funcs->decompress_buffer = squash_brotli_decompress_buffer;
    funcs->compress_buffer = squash_brotli_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
