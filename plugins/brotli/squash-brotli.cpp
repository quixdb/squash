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
 *   Eugene Kliuchnikov <eustas.ru+squash@gmail.com>
 */

#include <assert.h>

#include <squash/squash.h>

#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"

enum SquashBrotliOptionIndex {
  SQUASH_BROTLI_OPT_LEVEL = 0,
  SQUASH_BROTLI_OPT_MODE
};

/* C++ doesn't allow us to initialize this correctly here (or, at
   least, I can't figure out how to do it), so there is some extra
   code in the init_plugin func to finish it off. */
static SquashOptionInfo squash_brotli_options[] = {
  { "level", SQUASH_OPTION_TYPE_RANGE_INT },
  { "mode", SQUASH_OPTION_TYPE_ENUM_STRING },
  { NULL, SQUASH_OPTION_TYPE_NONE }
};

typedef struct SquashBrotliStream_s {
  SquashStream base_object;

  BrotliInput in;
  BrotliOutput out;
  BrotliState* decompressor;
  bool finished;
  brotli::BrotliCompressor* compressor;
  bool should_flush;
  bool should_seal;
  size_t remaining_block_in;
  size_t remaining_out;
  uint8_t* next_out;
  uint8_t seal_out[6];
} SquashBrotliStream;

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus                squash_plugin_init_plugin     (SquashPlugin* plugin);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus                squash_plugin_init_codec      (SquashCodec* codec, SquashCodecImpl* impl);

static void                 squash_brotli_stream_init     (SquashBrotliStream* stream,
                                                           SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashOptions* options,
                                                           SquashDestroyNotify destroy_notify);
static SquashBrotliStream*  squash_brotli_stream_new      (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashOptions* options);
static void                 squash_brotli_stream_destroy  (void* stream);
static void                 squash_brotli_stream_free     (void* stream);

static SquashBrotliStream*
squash_brotli_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
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
                           SquashOptions* options,
                           SquashDestroyNotify destroy_notify) {
  SquashStream* stream = (SquashStream*) s;
  squash_stream_init (stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  s->finished = false;
  if (stream_type == SQUASH_STREAM_COMPRESS) {
    brotli::BrotliParams params;
    params.quality = squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_BROTLI_OPT_LEVEL);
    params.mode = (brotli::BrotliParams::Mode) squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_BROTLI_OPT_MODE);
    s->compressor = new brotli::BrotliCompressor (params);
    s->remaining_block_in = s->compressor->input_block_size();
    s->remaining_out = 0;
    s->next_out = NULL;
    s->should_flush = false;
    s->should_seal = false;
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    s->decompressor = new BrotliState ();
    BrotliStateInit(s->decompressor);
  } else {
    squash_assert_unreachable();
  }
}

static void
squash_brotli_stream_destroy (void* stream) {
  SquashBrotliStream* s = (SquashBrotliStream*) stream;

  if (((SquashStream*) stream)->stream_type == SQUASH_STREAM_COMPRESS) {
    delete s->compressor;
  } else if (((SquashStream*) stream)->stream_type == SQUASH_STREAM_DECOMPRESS) {
    BrotliStateCleanup(s->decompressor);
    delete s->decompressor;
  } else {
    squash_assert_unreachable();
  }

  squash_stream_destroy (stream);
}

static void
squash_brotli_stream_free (void* stream) {
  squash_brotli_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_brotli_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_brotli_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_brotli_status_to_squash_status (BrotliResult status) {
  switch (status) {
    case BROTLI_RESULT_SUCCESS:
      return SQUASH_OK;
    case BROTLI_RESULT_NEEDS_MORE_INPUT:
      return squash_error (SQUASH_BUFFER_EMPTY);
    case BROTLI_RESULT_NEEDS_MORE_OUTPUT:
      return squash_error (SQUASH_BUFFER_FULL);
    case BROTLI_RESULT_ERROR:
    default:
      return squash_error (SQUASH_FAILED);
  }
}

static SquashStatus
squash_brotli_compress_stream (SquashStream* stream, SquashOperation operation) {
  SquashBrotliStream* s = (SquashBrotliStream*) stream;
  if (operation == SQUASH_OPERATION_FLUSH) {
    s->should_flush = true;
  }

  bool end_of_input = false;
  while (true) {
    size_t out_size = s->remaining_out;
    if (out_size != 0) {
      out_size = (s->base_object.avail_out < out_size) ? s->base_object.avail_out : out_size;
      memcpy (s->base_object.next_out, s->next_out, out_size);
      s->base_object.next_out += out_size;
      s->base_object.avail_out -= out_size;
      s->next_out += out_size;
      s->remaining_out -= out_size;
      if (s->base_object.avail_out == 0) {
        return SQUASH_PROCESSING;
      }
    }

    if (s->should_seal) {
      s->remaining_out = 6;
      s->next_out = s->seal_out;
      s->should_seal = false;
      end_of_input = true;
      if (SQUASH_UNLIKELY(!s->compressor->WriteMetadata(0, NULL, false, &s->remaining_out, s->next_out))) {
        return squash_error (SQUASH_FAILED);
      }
      continue;
    }

    if (end_of_input || s->finished) {
      return SQUASH_OK;
    }

    size_t in_size = s->base_object.avail_in;
    in_size = (in_size < s->remaining_block_in) ? in_size : s->remaining_block_in;
    if (in_size != 0) {
      s->compressor->CopyInputToRingBuffer (in_size, s->base_object.next_in);
      s->base_object.next_in += in_size;
      s->base_object.avail_in -= in_size;
      s->remaining_block_in -= in_size;
      if (s->remaining_block_in == 0) {
        s->remaining_block_in = s->compressor->input_block_size ();
      }
    }

    end_of_input = s->base_object.avail_in == 0;
    bool is_last = (operation == SQUASH_OPERATION_FINISH) && end_of_input;
    if (SQUASH_UNLIKELY(!s->compressor->WriteBrotliData(is_last,
                                                        s->should_flush && end_of_input,
                                                        &s->remaining_out, &s->next_out))) {
      return squash_error (SQUASH_FAILED);
    }
    if (is_last) {
      s->finished = true;
    } else if (s->should_flush && end_of_input) {
      s->should_flush = false;
      s->should_seal = true;
    }
  }
}

static SquashStatus
squash_brotli_decompress_stream (SquashStream* stream, SquashOperation operation) {
  SquashBrotliStream* s = (SquashBrotliStream*) stream;

  if (s->finished) {
    return SQUASH_OK;
  }

  try {
    size_t total_out;
    BrotliResult res = BrotliDecompressStream (&s->base_object.avail_in, &s->base_object.next_in,
                                               &s->base_object.avail_out, &s->base_object.next_out,
                                               &total_out, s->decompressor);
    if (res == BROTLI_RESULT_SUCCESS) {
      s->finished = true;
      return SQUASH_OK;
    }
    if (SQUASH_LIKELY(res ==  BROTLI_RESULT_NEEDS_MORE_OUTPUT || res == BROTLI_RESULT_NEEDS_MORE_INPUT)) {
      return (res == BROTLI_RESULT_NEEDS_MORE_OUTPUT) ? SQUASH_PROCESSING : SQUASH_OK;
    }
    return squash_error (SQUASH_FAILED);
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }
}

static SquashStatus
squash_brotli_process_stream (SquashStream* stream, SquashOperation operation) {
  if (stream->stream_type == SQUASH_STREAM_COMPRESS)
    return squash_brotli_compress_stream (stream, operation);
  else
    return squash_brotli_decompress_stream (stream, operation);
}

static size_t
squash_brotli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return uncompressed_size + 5;
}

static SquashStatus
squash_brotli_decompress_buffer (SquashCodec* codec,
                                 size_t* decompressed_size,
                                 uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                 size_t compressed_size,
                                 const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                 SquashOptions* options) {
  try {
    BrotliResult res = BrotliDecompressBuffer (compressed_size, compressed,
                                               decompressed_size, decompressed);
    return squash_brotli_status_to_squash_status (res);
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }
}

static SquashStatus
squash_brotli_compress_buffer (SquashCodec* codec,
                               size_t* compressed_size,
                               uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                               size_t uncompressed_size,
                               const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                               SquashOptions* options) {
  brotli::BrotliParams params;
  params.quality = squash_codec_get_option_int_index (codec, options, SQUASH_BROTLI_OPT_LEVEL);
  params.mode = (brotli::BrotliParams::Mode) squash_codec_get_option_int_index (codec, options, SQUASH_BROTLI_OPT_MODE);
  try {
    int res = brotli::BrotliCompressBuffer (params,
                                            uncompressed_size, uncompressed,
                                            compressed_size, compressed);
    return SQUASH_LIKELY(res == 1) ? SQUASH_OK : squash_error (SQUASH_FAILED);
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }
}

extern "C" SquashStatus
squash_plugin_init_plugin (SquashPlugin* plugin) {
  const SquashOptionInfoRangeInt level_range = { 1, 11, 0, false };
  squash_brotli_options[SQUASH_BROTLI_OPT_LEVEL].default_value.int_value = 11;
  squash_brotli_options[SQUASH_BROTLI_OPT_LEVEL].info.range_int = level_range;

  static const SquashOptionInfoEnumStringMap option_strings[] = {
      { "generic", brotli::BrotliParams::MODE_GENERIC },
      { "text", brotli::BrotliParams::MODE_TEXT },
      { "font", brotli::BrotliParams::MODE_FONT },
      { NULL, 0 }
  };
  squash_brotli_options[SQUASH_BROTLI_OPT_MODE].default_value.int_value = brotli::BrotliParams::MODE_GENERIC;
  squash_brotli_options[SQUASH_BROTLI_OPT_MODE].info.enum_string.values = option_strings;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("brotli", name) == 0)) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->options = squash_brotli_options;
    impl->get_max_compressed_size = squash_brotli_get_max_compressed_size;
    impl->create_stream = squash_brotli_create_stream;
    impl->process_stream = squash_brotli_process_stream;
    impl->decompress_buffer = squash_brotli_decompress_buffer;
    impl->compress_buffer = squash_brotli_compress_buffer;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
