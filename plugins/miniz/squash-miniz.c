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
  switch (name[0]) {
    case 'z':
      return SQUASH_MINIZ_TYPE_ZLIB;
    case 'g':
      return SQUASH_MINIZ_TYPE_GZIP;
    case 'd':
      return SQUASH_MINIZ_TYPE_DEFLATE;
  }
  HEDLEY_UNREACHABLE();
}

static bool
squash_miniz_init_stream (SquashStream* stream, SquashStreamType stream_type, SquashOptions* options, void* priv) {
  mz_stream* s = priv;
  SquashCodec* codec = stream->codec;
  SquashMinizType type = squash_miniz_codec_to_type (codec);

  const mz_stream tmp = { 0, };
  *s = tmp;

  s->zalloc = squash_zlib_malloc;
  s->zfree  = squash_zlib_free;

  int miniz_e = 0;
  int window_bits;

  window_bits = squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_WINDOW_BITS);
  if (type == SQUASH_MINIZ_TYPE_DEFLATE) {
    window_bits = -window_bits;
  } else if (type == SQUASH_MINIZ_TYPE_GZIP) {
    window_bits += 16;
  }

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    miniz_e = mz_deflateInit2 (s,
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_LEVEL),
                               MZ_DEFLATED,
                               window_bits,
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_MEM_LEVEL),
                               squash_options_get_int_at (options, codec, SQUASH_MINIZ_OPT_STRATEGY));
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    miniz_e = mz_inflateInit2 (s, window_bits);
  } else {
    HEDLEY_UNREACHABLE();
  }

  if (miniz_e != MZ_OK) {
    return false;
  }

  return true;
}

static void
squash_miniz_destroy_stream (SquashStream* stream, void* priv) {
  mz_stream* s = priv;

  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      mz_deflateEnd (s);
      break;
    case SQUASH_STREAM_DECOMPRESS:
      mz_inflateEnd (s);
      break;
  }
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
    default:
      HEDLEY_UNREACHABLE();
      break;
  }

  HEDLEY_UNREACHABLE ();
}

static SquashStatus
squash_miniz_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  mz_stream* s = priv;
  int miniz_e;
  SquashStatus res = SQUASH_FAILED;

#if UINT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(UINT_MAX < stream->avail_in) ||
      HEDLEY_UNLIKELY(UINT_MAX < stream->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_MINIZ_STREAM_COPY_TO_MINIZ_STREAM(stream, s);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    miniz_e = mz_deflate (s, squash_operation_to_miniz (operation));
  } else if (stream->stream_type == SQUASH_STREAM_DECOMPRESS) {
    miniz_e = mz_inflate (s, squash_operation_to_miniz (operation));
  } else {
    HEDLEY_UNREACHABLE();
  }

#if SIZE_MAX < UINT_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < s->avail_out) ||
      HEDLEY_UNLIKELY(SIZE_MAX < s->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_MINIZ_STREAM_COPY_FROM_MINIZ_STREAM(stream, s);

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
      return (squash_error (SQUASH_FAILED), 0);
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
    impl->priv_size = sizeof(mz_stream);
    impl->init_stream = squash_miniz_init_stream;
    impl->destroy_stream = squash_miniz_destroy_stream;
    impl->process_stream = squash_miniz_process_stream;
    impl->get_max_compressed_size = squash_miniz_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
