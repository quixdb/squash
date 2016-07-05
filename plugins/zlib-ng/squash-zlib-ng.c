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

#include <zlib.h>

typedef enum SquashZlibType_e {
  SQUASH_ZLIB_TYPE_ZLIB,
  SQUASH_ZLIB_TYPE_GZIP,
  SQUASH_ZLIB_TYPE_DEFLATE
} SquashZlibType;

typedef struct SquashZlibStream_s {
  SquashStream base_object;

  SquashZlibType type;
  z_stream stream;
} SquashZlibStream;

#define SQUASH_ZLIB_DEFAULT_LEVEL 6
#define SQUASH_ZLIB_DEFAULT_WINDOW_BITS 15
#define SQUASH_ZLIB_DEFAULT_MEM_LEVEL 8
#define SQUASH_ZLIB_DEFAULT_STRATEGY Z_DEFAULT_STRATEGY

enum SquashZlibOptIndex {
  SQUASH_ZLIB_OPT_LEVEL = 0,
  SQUASH_ZLIB_OPT_WINDOW_BITS,
  SQUASH_ZLIB_OPT_MEM_LEVEL,
  SQUASH_ZLIB_OPT_STRATEGY
};

static SquashOptionInfo squash_zlib_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = SQUASH_ZLIB_DEFAULT_LEVEL },
  { "window-bits",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 8,
      .max = 15 },
    .default_value.int_value = SQUASH_ZLIB_DEFAULT_WINDOW_BITS },
  { "mem-level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = SQUASH_ZLIB_DEFAULT_MEM_LEVEL },
  { "strategy",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "default", Z_DEFAULT_STRATEGY },
        { "filtered", Z_FILTERED },
        { "huffman", Z_HUFFMAN_ONLY },
        { "rle", Z_RLE },
        { "fixed", Z_FIXED },
        { NULL, 0 } } },
    .default_value.int_value = SQUASH_ZLIB_DEFAULT_STRATEGY },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);

static SquashZlibType     squash_zlib_codec_to_type  (SquashCodec* codec);

static void               squash_zlib_stream_init    (SquashZlibStream* stream,
                                                      SquashCodec* codec,
                                                      SquashStreamType stream_type,
                                                      SquashOptions* options,
                                                      SquashDestroyNotify destroy_notify);
static SquashZlibStream*  squash_zlib_stream_new     (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void               squash_zlib_stream_destroy (void* stream);

static voidpf
squash_zlib_malloc (voidpf opaque, uInt items, uInt size) {
  return (voidpf) squash_malloc (((size_t) items) * ((size_t) size));
}

static void
squash_zlib_free (voidpf opaque, voidpf address) {
  squash_free ((void*) address);
}

static SquashZlibType squash_zlib_codec_to_type (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);
  if (strcmp ("gzip", name) == 0) {
    return SQUASH_ZLIB_TYPE_GZIP;
  } else if (strcmp ("zlib", name) == 0) {
    return SQUASH_ZLIB_TYPE_ZLIB;
  } else if (strcmp ("deflate", name) == 0) {
    return SQUASH_ZLIB_TYPE_DEFLATE;
  } else {
    squash_assert_unreachable();
  }
}

static void
squash_zlib_stream_init (SquashZlibStream* stream,
                         SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  z_stream tmp = { 0, };
  stream->stream = tmp;
  stream->stream.zalloc = squash_zlib_malloc;
  stream->stream.zfree  = squash_zlib_free;
}

static void
squash_zlib_stream_destroy (void* stream) {
  switch (((SquashStream*) stream)->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      deflateEnd (&(((SquashZlibStream*) stream)->stream));
      break;
    case SQUASH_STREAM_DECOMPRESS:
      inflateEnd (&(((SquashZlibStream*) stream)->stream));
      break;
  }

  squash_stream_destroy (stream);
}

static SquashZlibStream*
squash_zlib_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  int zlib_e = 0;
  SquashZlibStream* stream;
  int window_bits;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = squash_malloc (sizeof (SquashZlibStream));
  squash_zlib_stream_init (stream, codec, stream_type, options, squash_zlib_stream_destroy);

  stream->type = squash_zlib_codec_to_type (codec);

  window_bits = squash_options_get_int_at (options, codec, SQUASH_ZLIB_OPT_WINDOW_BITS);
  if (stream->type == SQUASH_ZLIB_TYPE_DEFLATE) {
    window_bits = -window_bits;
  } else if (stream->type == SQUASH_ZLIB_TYPE_GZIP) {
    window_bits += 16;
  }

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    zlib_e = deflateInit2 (&(stream->stream),
                           squash_options_get_int_at (options, codec, SQUASH_ZLIB_OPT_LEVEL),
                           Z_DEFLATED,
                           window_bits,
                           squash_options_get_int_at (options, codec, SQUASH_ZLIB_OPT_MEM_LEVEL),
                           squash_options_get_int_at (options, codec, SQUASH_ZLIB_OPT_STRATEGY));
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    zlib_e = inflateInit2 (&(stream->stream), window_bits);
  } else {
    squash_assert_unreachable();
  }

  if (zlib_e != Z_OK) {
    stream = squash_object_unref (stream);
  }

  return stream;
}

static SquashStream*
squash_zlib_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_zlib_stream_new (codec, stream_type, options);
}

#define SQUASH_ZLIB_STREAM_COPY_TO_ZLIB_STREAM(stream,zlib_stream) \
  zlib_stream->next_in = (Bytef*) stream->next_in; \
  zlib_stream->avail_in = (uInt) stream->avail_in; \
  zlib_stream->next_out = (Bytef*) stream->next_out; \
  zlib_stream->avail_out = (uInt) stream->avail_out

#define SQUASH_ZLIB_STREAM_COPY_FROM_ZLIB_STREAM(stream,zlib_stream) \
  stream->next_in = (uint8_t*) zlib_stream->next_in;       \
  stream->avail_in = (size_t) zlib_stream->avail_in;            \
  stream->next_out = (uint8_t*) zlib_stream->next_out;    \
  stream->avail_out = (size_t) zlib_stream->avail_out

static int
squash_operation_to_zlib (SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return Z_NO_FLUSH;
    case SQUASH_OPERATION_FLUSH:
      return Z_SYNC_FLUSH;
    case SQUASH_OPERATION_FINISH:
      return Z_FINISH;
    case SQUASH_OPERATION_TERMINATE:
      squash_assert_unreachable();
      break;
  }

  squash_assert_unreachable ();
}

static SquashStatus
squash_zlib_process_stream (SquashStream* stream, SquashOperation operation) {
  z_stream* zlib_stream;
  int zlib_e;
  SquashStatus res = SQUASH_FAILED;

  assert (stream != NULL);

  zlib_stream = &(((SquashZlibStream*) stream)->stream);

#if UINT_MAX < SIZE_MAX
  if (HEDLEY_UNLIKELY(UINT_MAX < stream->avail_in) ||
      HEDLEY_UNLIKELY(UINT_MAX < stream->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_ZLIB_STREAM_COPY_TO_ZLIB_STREAM(stream, zlib_stream);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    zlib_e = deflate (zlib_stream, squash_operation_to_zlib (operation));
  } else {
    zlib_e = inflate (zlib_stream, squash_operation_to_zlib (operation));
  }

#if SIZE_MAX < UINT_MAX
  if (HEDLEY_UNLIKELY(SIZE_MAX < zlib_stream->avail_out) ||
      HEDLEY_UNLIKELY(SIZE_MAX < zlib_stream->avail_out))
    return squash_error (SQUASH_RANGE);
#endif
  SQUASH_ZLIB_STREAM_COPY_FROM_ZLIB_STREAM(stream, zlib_stream);

  switch (zlib_e) {
    case Z_OK:
      switch (operation) {
        case SQUASH_OPERATION_PROCESS:
          res = (stream->avail_in == 0) ? SQUASH_OK : SQUASH_PROCESSING;
          break;
        case SQUASH_OPERATION_FLUSH:
        case SQUASH_OPERATION_FINISH:
          res = SQUASH_PROCESSING;
          break;
        case SQUASH_OPERATION_TERMINATE:
          squash_assert_unreachable ();
          break;
      }
      break;
    case Z_BUF_ERROR:
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
          squash_assert_unreachable ();
          break;
      }
      break;
    case Z_STREAM_END:
      res = SQUASH_OK;
      break;
    case Z_MEM_ERROR:
      res = SQUASH_MEMORY;
      break;
    default:
      res = SQUASH_FAILED;
      break;
  }

  return res;
}

static size_t
squash_zlib_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  SquashZlibType type = squash_zlib_codec_to_type (codec);

#if SIZE_MAX < ULONG_MAX
  if (HEDLEY_UNLIKELY(uncompressed_size > ULONG_MAX)) {
    squash_error (SQUASH_BUFFER_TOO_LARGE);
    return 0;
  }
#endif

  if (type == SQUASH_ZLIB_TYPE_ZLIB) {
    return (size_t) compressBound ((uLong) uncompressed_size);
  } else {
    z_stream stream = { 0, };
    size_t max_compressed_size;
    int zlib_e;

    int window_bits = 14;
    if (type == SQUASH_ZLIB_TYPE_DEFLATE) {
      window_bits = -window_bits;
    } else if (type == SQUASH_ZLIB_TYPE_GZIP) {
      window_bits += 16;
    }

    zlib_e = deflateInit2 (&stream,
                           SQUASH_ZLIB_DEFAULT_LEVEL,
                           Z_DEFLATED,
                           window_bits,
                           9,
                           SQUASH_ZLIB_DEFAULT_STRATEGY);
    if (zlib_e != Z_OK) {
      return 0;
    }

    max_compressed_size = (size_t) deflateBound (&stream, (uLong) uncompressed_size);

    deflateEnd (&stream);

    return max_compressed_size;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("gzip", name) == 0 ||
      strcmp ("zlib", name) == 0 ||
      strcmp ("deflate", name) == 0) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->options = squash_zlib_options;
    impl->create_stream = squash_zlib_create_stream;
    impl->process_stream = squash_zlib_process_stream;
    impl->get_max_compressed_size = squash_zlib_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
