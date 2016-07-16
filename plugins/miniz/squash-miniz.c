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
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <squash/squash.h>

#define MINIZ_NO_STDIO
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
//#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_NO_MALLOC
#include "miniz/miniz.c"

typedef enum SquashMinizType_e {
  SQUASH_MINIZ_TYPE_ZLIB,
  SQUASH_MINIZ_TYPE_GZIP,
  SQUASH_MINIZ_TYPE_DEFLATE
} SquashMinizType;

typedef struct SquashMinizStream_s {
  SquashStream base_object;

  SquashMinizType type;
  mz_stream stream;
} SquashMinizStream;

#define SQUASH_MINIZ_DEFAULT_LEVEL 6
#define SQUASH_MINIZ_DEFAULT_WINDOW_BITS 15
#define SQUASH_MINIZ_DEFAULT_MEM_LEVEL 8
#define SQUASH_MINIZ_DEFAULT_STRATEGY MZ_DEFAULT_STRATEGY

enum SquashMinizOptIndex {
  SQUASH_MINIZ_OPT_LEVEL = 0,
  SQUASH_MINIZ_OPT_WINDOW_BITS,
  SQUASH_MINIZ_OPT_MEM_LEVEL,
  SQUASH_MINIZ_OPT_STRATEGY
};

static SquashOptionInfo squash_miniz_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = SQUASH_MINIZ_DEFAULT_LEVEL },
  { "window-bits",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 8,
      .max = 15 },
    .default_value.int_value = SQUASH_MINIZ_DEFAULT_WINDOW_BITS },
  { "mem-level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = SQUASH_MINIZ_DEFAULT_MEM_LEVEL },
  { "strategy",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "default", MZ_DEFAULT_STRATEGY },
        { "filtered", MZ_FILTERED },
        { "huffman", MZ_HUFFMAN_ONLY },
        { "rle", MZ_RLE },
        { "fixed", MZ_FIXED },
        { NULL, 0 } } },
    .default_value.int_value = SQUASH_MINIZ_DEFAULT_STRATEGY },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);

static SquashMinizType    squash_miniz_codec_to_type  (SquashCodec* codec);

static void               squash_miniz_stream_init    (SquashMinizStream* stream,
                                                      SquashCodec* codec,
                                                      SquashStreamType stream_type,
                                                      SquashOptions* options,
                                                      SquashDestroyNotify destroy_notify);
static SquashMinizStream* squash_miniz_stream_new     (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void               squash_miniz_stream_destroy (void* stream);

static void*
squash_zlib_malloc (void* opaque, size_t items, size_t size) {
  return squash_malloc (items * size);
}

static void
squash_zlib_free (void* opaque, void* address) {
  squash_free (address);
}

static SquashMinizType squash_miniz_codec_to_type (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);
  if (strcmp ("gzip", name) == 0) {
    return SQUASH_MINIZ_TYPE_GZIP;
  } else if (strcmp ("zlib", name) == 0) {
    return SQUASH_MINIZ_TYPE_ZLIB;
  } else if (strcmp ("deflate", name) == 0) {
    return SQUASH_MINIZ_TYPE_DEFLATE;
  } else {
    HEDLEY_UNREACHABLE();
  }
}

static void
squash_miniz_stream_init (SquashMinizStream* stream,
                          SquashCodec* codec,
                          SquashStreamType stream_type,
                          SquashOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  mz_stream tmp = { 0, };
  stream->stream = tmp;
  stream->stream.zalloc = squash_zlib_malloc;
  stream->stream.zfree  = squash_zlib_free;
}

static void
squash_miniz_stream_destroy (void* stream) {
  switch (((SquashStream*) stream)->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      mz_deflateEnd (&(((SquashMinizStream*) stream)->stream));
      break;
    case SQUASH_STREAM_DECOMPRESS:
      mz_inflateEnd (&(((SquashMinizStream*) stream)->stream));
      break;
  }

  squash_stream_destroy (stream);
}

static SquashMinizStream*
squash_miniz_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  int miniz_e = 0;
  SquashMinizStream* stream;
  int window_bits;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = squash_malloc (sizeof (SquashMinizStream));
  squash_miniz_stream_init (stream, codec, stream_type, options, squash_miniz_stream_destroy);

  stream->type = squash_miniz_codec_to_type (codec);

  window_bits = squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_WINDOW_BITS);
  if (stream->type == SQUASH_MINIZ_TYPE_DEFLATE) {
    window_bits = -window_bits;
  } else if (stream->type == SQUASH_MINIZ_TYPE_GZIP) {
    window_bits += 16;
  }

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    miniz_e = mz_deflateInit2 (&(stream->stream),
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_LEVEL),
                               MZ_DEFLATED,
                               window_bits,
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_MEM_LEVEL),
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_STRATEGY));
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    miniz_e = mz_inflateInit2 (&(stream->stream), window_bits);
  } else {
    HEDLEY_UNREACHABLE();
  }

  if (miniz_e != MZ_OK) {
    stream = squash_object_unref (stream);
  }

  return stream;
}

static SquashStream*
squash_miniz_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_miniz_stream_new (codec, stream_type, options);
}

#define SQUASH_MINIZ_STREAM_COPY_TO_MINIZ_STREAM(stream,miniz_stream) \
  miniz_stream->next_in = stream->next_in;                            \
  miniz_stream->avail_in = stream->avail_in;                          \
  miniz_stream->next_out = stream->next_out;                          \
  miniz_stream->avail_out = stream->avail_out

#define SQUASH_MINIZ_STREAM_COPY_FROM_MINIZ_STREAM(stream,miniz_stream) \
  stream->next_in = (uint8_t*) miniz_stream->next_in;                   \
  stream->avail_in = (size_t) miniz_stream->avail_in;                   \
  stream->next_out = (uint8_t*) miniz_stream->next_out;                 \
  stream->avail_out = (size_t) miniz_stream->avail_out

static int
squash_operation_to_miniz (SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return MZ_NO_FLUSH;
    case SQUASH_OPERATION_FLUSH:
      return MZ_SYNC_FLUSH;
    case SQUASH_OPERATION_FINISH:
      return MZ_FINISH;
    case SQUASH_OPERATION_TERMINATE:
      HEDLEY_UNREACHABLE();
      break;
  }

  HEDLEY_UNREACHABLE ();
}

static SquashStatus
squash_miniz_process_stream (SquashStream* stream, SquashOperation operation) {
  mz_stream* miniz_stream;
  int miniz_e;
  SquashStatus res = SQUASH_FAILED;

  assert (stream != NULL);

  miniz_stream = &(((SquashMinizStream*) stream)->stream);

#if UINT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(UINT_MAX < stream->avail_in) ||
      HEDLEY_UNLIKELY(UINT_MAX < stream->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_MINIZ_STREAM_COPY_TO_MINIZ_STREAM(stream, miniz_stream);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    miniz_e = mz_deflate (miniz_stream, squash_operation_to_miniz (operation));
  } else {
    miniz_e = mz_inflate (miniz_stream, squash_operation_to_miniz (operation));
  }

#if SIZE_MAX < UINT_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < miniz_stream->avail_out) ||
      HEDLEY_UNLIKELY(SIZE_MAX < miniz_stream->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_MINIZ_STREAM_COPY_FROM_MINIZ_STREAM(stream, miniz_stream);

  switch (miniz_e) {
    case MZ_OK:
      switch (operation) {
        case SQUASH_OPERATION_PROCESS:
          res = (stream->avail_in == 0) ? SQUASH_OK : SQUASH_PROCESSING;
          break;
        case SQUASH_OPERATION_FLUSH:
        case SQUASH_OPERATION_FINISH:
          res = SQUASH_PROCESSING;
          break;
        case SQUASH_OPERATION_TERMINATE:
          HEDLEY_UNREACHABLE ();
          break;
      }
      break;
    case MZ_BUF_ERROR:
      switch (operation) {
        case SQUASH_OPERATION_PROCESS:
          res = (stream->avail_in == 0) ? SQUASH_OK : SQUASH_BUFFER_FULL;
          break;
        case SQUASH_OPERATION_FLUSH:
        case SQUASH_OPERATION_FINISH:
          if (stream->avail_in == 0) {
            if (stream->avail_out == 0) {
              res = SQUASH_PROCESSING;
            } else {
              res = SQUASH_OK;
            }
          } else {
            res = SQUASH_PROCESSING;
          }
          break;
        case SQUASH_OPERATION_TERMINATE:
          HEDLEY_UNREACHABLE ();
          break;
      }
      break;
    case MZ_STREAM_END:
      res = SQUASH_OK;
      break;
    case MZ_MEM_ERROR:
      res = SQUASH_MEMORY;
      break;
    default:
      res = SQUASH_FAILED;
      break;
  }

  return res;
}

static size_t
squash_miniz_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  SquashMinizType type = squash_miniz_codec_to_type (codec);

#if SIZE_MAX < ULONG_MAX
  if (HEDLEY_UNLIKELY(uncompressed_size > ULONG_MAX)) {
    squash_error (SQUASH_BUFFER_TOO_LARGE);
    return 0;
  }
#endif

  if (type == SQUASH_MINIZ_TYPE_ZLIB) {
    return (size_t) mz_compressBound ((mz_ulong) uncompressed_size);
  } else {
    mz_stream stream = { 0, };
    size_t max_compressed_size;
    int miniz_e;

    int window_bits = 14;
    if (type == SQUASH_MINIZ_TYPE_DEFLATE) {
      window_bits = -window_bits;
    } else if (type == SQUASH_MINIZ_TYPE_GZIP) {
      window_bits += 16;
    }

    miniz_e = mz_deflateInit2 (&stream,
                               SQUASH_MINIZ_DEFAULT_LEVEL,
                               MZ_DEFLATED,
                               window_bits,
                               9,
                               SQUASH_MINIZ_DEFAULT_STRATEGY);
    if (miniz_e != MZ_OK) {
      return 0;
    }

    max_compressed_size = (size_t) mz_deflateBound (&stream, (mz_ulong) uncompressed_size);

    mz_deflateEnd (&stream);

    return max_compressed_size;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zlib", name) == 0) {
    /* See https://github.com/richgel999/miniz/issues/48 */
    /* impl->info = SQUASH_CODEC_INFO_CAN_FLUSH; */
    impl->options = squash_miniz_options;
    impl->create_stream = squash_miniz_create_stream;
    impl->process_stream = squash_miniz_process_stream;
    impl->get_max_compressed_size = squash_miniz_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
