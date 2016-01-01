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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <squash/squash.h>

#include <lzma.h>

typedef enum SquashLZMAType_e {
  SQUASH_LZMA_TYPE_LZMA = 1,
  SQUASH_LZMA_TYPE_XZ,
  SQUASH_LZMA_TYPE_LZMA1,
  SQUASH_LZMA_TYPE_LZMA2
} SquashLZMAType;

typedef struct SquashLZMAStream_s {
  SquashStream base_object;

  SquashLZMAType type;
  lzma_stream stream;
  lzma_allocator allocator;
} SquashLZMAStream;

enum SquashLZMAOptIndex {
  SQUASH_LZMA_OPT_LEVEL = 0,
  SQUASH_LZMA_OPT_DICT_SIZE,
  SQUASH_LZMA_OPT_LC,
  SQUASH_LZMA_OPT_LP,
  SQUASH_LZMA_OPT_PB,
  SQUASH_LZMA_OPT_MEM_LIMIT,
  SQUASH_LZMA_OPT_CHECK,
};

static SquashOptionInfo squash_lzma_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 6 },
  { "dict-size",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 4096,
      .max = 1610612736 },
    .default_value.size_value = 8388608 },
  { "lc",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 3 },
  { "lp",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 0 },
  { "pb",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 2 },
  { "mem-limit",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 0,
      .max = SIZE_MAX },
    .default_value.size_value = (1024 * 1024 * 140) },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static SquashOptionInfo squash_lzma12_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 6 },
  { "dict-size",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 4096,
      .max = 1610612736 },
    .default_value.size_value = 8388608 },
  { "lc",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 3 },
  { "lp",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 0 },
  { "pb",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 2 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static SquashOptionInfo squash_lzma_xz_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 6 },
  { "dict-size",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 4096,
      .max = 1610612736 },
    .default_value.size_value = 8388608 },
  { "lc",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 3 },
  { "lp",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 0 },
  { "pb",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 4 },
    .default_value.int_value = 2 },
  { "mem-limit",
    SQUASH_OPTION_TYPE_RANGE_SIZE,
    .info.range_size = {
      .min = 0,
      .max = SIZE_MAX },
    .default_value.size_value = SIZE_MAX },
  { "check",
    SQUASH_OPTION_TYPE_ENUM_STRING,
    .info.enum_string = {
      .values = (const SquashOptionInfoEnumStringMap []) {
        { "none", LZMA_CHECK_NONE },
        { "crc32", LZMA_CHECK_CRC32 },
        { "crc64", LZMA_CHECK_CRC64 },
        { "sha256", LZMA_CHECK_SHA256 },
        { NULL, 0 } } },
    .default_value.int_value = LZMA_CHECK_CRC64 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecImpl* impl);

static SquashLZMAType     squash_lzma_codec_to_type   (SquashCodec* codec);

static void               squash_lzma_stream_init     (SquashLZMAStream* stream,
                                                       SquashCodec* codec,
                                                       SquashLZMAType type,
                                                       SquashStreamType stream_type,
                                                       SquashOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashLZMAStream*  squash_lzma_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void               squash_lzma_stream_destroy  (void* stream);
static void               squash_lzma_stream_free     (void* stream);

static SquashLZMAType squash_lzma_codec_to_type (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);
  if (strcmp ("xz", name) == 0) {
    return SQUASH_LZMA_TYPE_XZ;
  } else if (strcmp ("lzma2", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA2;
  } else if (strcmp ("lzma", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA;
  } else if (strcmp ("lzma1", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA1;
  } else {
    return (SquashLZMAType) 0;
  }
}

static void* squash_lzma_calloc (void *opaque, size_t nmemb, size_t size) {
  SquashContext* ctx = (SquashContext*) opaque;
  void* ptr = squash_malloc (ctx, nmemb * size);
  if (SQUASH_UNLIKELY(ptr == NULL))
    return ptr;
  return memset (ptr, 0, nmemb * size);
}

static void squash_lzma_free (void *opaque, void* ptr) {
  SquashContext* ctx = (SquashContext*) opaque;
  squash_free (ctx, ptr);
}

static void
squash_lzma_stream_init (SquashLZMAStream* stream,
                         SquashCodec* codec,
                         SquashLZMAType type,
                         SquashStreamType stream_type,
                         SquashOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  stream->allocator.opaque = squash_codec_get_context (codec);
  stream->allocator.alloc = squash_lzma_calloc;
  stream->allocator.free = squash_lzma_free;

  lzma_stream s = LZMA_STREAM_INIT;
  s.allocator = &(stream->allocator);

  stream->stream = s;
  stream->type = type;
}

static void
squash_lzma_stream_destroy (void* stream) {
  lzma_end (&((SquashLZMAStream*) stream)->stream);
  squash_stream_destroy (stream);
}

static void
squash_lzma_stream_free (void* stream) {
  squash_lzma_stream_destroy (stream);
  SquashContext* ctx = squash_codec_get_context (((SquashStream*) stream)->codec);
  squash_free (ctx, stream);
}

static SquashLZMAStream*
squash_lzma_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  lzma_ret lzma_e;
  SquashLZMAStream* stream;
  SquashLZMAType lzma_type;
  lzma_options_lzma lzma_options = { 0, };
  lzma_filter filters[2];
  SquashContext* ctx;

  assert (codec != NULL);

  ctx = squash_codec_get_context (codec);

  lzma_type = squash_lzma_codec_to_type (codec);

  lzma_lzma_preset (&lzma_options, (uint32_t) squash_codec_get_option_int_index (codec, options, SQUASH_LZMA_OPT_LEVEL));
  lzma_options.lc = squash_codec_get_option_int_index (codec, options, SQUASH_LZMA_OPT_LC);
  lzma_options.lp = squash_codec_get_option_int_index (codec, options, SQUASH_LZMA_OPT_LP);
  lzma_options.pb = squash_codec_get_option_int_index (codec, options, SQUASH_LZMA_OPT_PB);

  filters[0].options = &(lzma_options);

  switch (lzma_type) {
    case SQUASH_LZMA_TYPE_XZ:
    case SQUASH_LZMA_TYPE_LZMA2:
      filters[0].id = LZMA_FILTER_LZMA2;
      break;
    case SQUASH_LZMA_TYPE_LZMA:
    case SQUASH_LZMA_TYPE_LZMA1:
      filters[0].id = LZMA_FILTER_LZMA1;
      break;
  }

  filters[1].id = LZMA_VLI_UNKNOWN;
  filters[1].options = NULL;

  stream = (SquashLZMAStream*) squash_malloc (ctx, sizeof (SquashLZMAStream));
  squash_lzma_stream_init (stream, codec, lzma_type, stream_type, options, squash_lzma_stream_free);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    if (lzma_type == SQUASH_LZMA_TYPE_XZ) {
      lzma_e = lzma_stream_encoder (&(stream->stream), filters, (lzma_check) squash_codec_get_option_int_index (codec, options, SQUASH_LZMA_OPT_CHECK));
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA) {
      lzma_e = lzma_alone_encoder (&(stream->stream), filters[0].options);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA1 ||
               lzma_type == SQUASH_LZMA_TYPE_LZMA2) {
      lzma_e = lzma_raw_encoder(&(stream->stream), filters);
    } else {
      squash_assert_unreachable();
    }
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    if (lzma_type == SQUASH_LZMA_TYPE_XZ) {
      const uint64_t memlimit = squash_codec_get_option_size_index (codec, options, SQUASH_LZMA_OPT_MEM_LIMIT);
      lzma_e = lzma_stream_decoder(&(stream->stream), memlimit, 0);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA) {
      const uint64_t memlimit = squash_codec_get_option_size_index (codec, options, SQUASH_LZMA_OPT_MEM_LIMIT);
      lzma_e = lzma_alone_decoder(&(stream->stream), memlimit);
      assert (lzma_e == LZMA_OK);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA1 ||
               lzma_type == SQUASH_LZMA_TYPE_LZMA2) {
      lzma_e = lzma_raw_decoder(&(stream->stream), filters);
    } else {
      squash_assert_unreachable();
    }
  } else {
    squash_assert_unreachable();
  }

  if (lzma_e != LZMA_OK) {
    stream = squash_object_unref (stream);
  }

  return stream;
}

static SquashStream*
squash_lzma_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_lzma_stream_new (codec, stream_type, options);
}

#define SQUASH_LZMA_STREAM_COPY_TO_LZMA_STREAM(stream,lzma_stream)  \
  lzma_stream->next_in =  stream->next_in;                          \
  lzma_stream->avail_in = stream->avail_in;                         \
  lzma_stream->next_out = stream->next_out;                         \
  lzma_stream->avail_out = stream->avail_out

#define SQUASH_LZMA_STREAM_COPY_FROM_LZMA_STREAM(stream,lzma_stream)  \
  stream->next_in = lzma_stream->next_in;                             \
  stream->avail_in = lzma_stream->avail_in;                           \
  stream->next_out = lzma_stream->next_out;                           \
  stream->avail_out = lzma_stream->avail_out

static SquashStatus
squash_lzma_process_stream (SquashStream* stream, SquashOperation operation) {
  lzma_stream* s;
  lzma_ret lzma_e = LZMA_OK;

  assert (stream != NULL);
  s = &(((SquashLZMAStream*) stream)->stream);

  SQUASH_LZMA_STREAM_COPY_TO_LZMA_STREAM(stream, s);
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      lzma_e = lzma_code (s, LZMA_RUN);
      break;
    case SQUASH_OPERATION_FLUSH:
      lzma_e = lzma_code (s, LZMA_SYNC_FLUSH);
      break;
    case SQUASH_OPERATION_FINISH:
      lzma_e = lzma_code (s, LZMA_FINISH);
      break;
    case SQUASH_OPERATION_TERMINATE:
      squash_assert_unreachable ();
  }
  SQUASH_LZMA_STREAM_COPY_FROM_LZMA_STREAM(stream, s);

  if (lzma_e == LZMA_OK) {
    switch (operation) {
      case SQUASH_OPERATION_PROCESS:
        return (stream->avail_in == 0) ? SQUASH_OK : SQUASH_PROCESSING;
        break;
      case SQUASH_OPERATION_FLUSH:
        return SQUASH_OK;
        break;
      case SQUASH_OPERATION_FINISH:
        return SQUASH_PROCESSING;
        break;
      case SQUASH_OPERATION_TERMINATE:
        squash_assert_unreachable ();
        break;
    }
  } else if (lzma_e == LZMA_STREAM_END) {
    return SQUASH_OK;
  } else {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

static size_t
squash_lzma_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  SquashLZMAType lzma_type = squash_lzma_codec_to_type (codec);

  switch (lzma_type) {
    case SQUASH_LZMA_TYPE_XZ:
    case SQUASH_LZMA_TYPE_LZMA2:
      return lzma_stream_buffer_bound (uncompressed_size) + (uncompressed_size / (256 * 1024));
      break;
    case SQUASH_LZMA_TYPE_LZMA:
    case SQUASH_LZMA_TYPE_LZMA1:
      return (uncompressed_size / 56) + uncompressed_size + 48;
      break;
  }

  squash_assert_unreachable ();
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  impl->options = squash_lzma_options;

  switch (squash_lzma_codec_to_type (codec)) {
    case SQUASH_LZMA_TYPE_XZ:
      impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
      impl->options = squash_lzma_xz_options;
      break;
    case SQUASH_LZMA_TYPE_LZMA2:
      impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
      impl->options = squash_lzma12_options;
      break;
    case SQUASH_LZMA_TYPE_LZMA:
      impl->options = squash_lzma_options;
      break;
    case SQUASH_LZMA_TYPE_LZMA1:
      impl->options = squash_lzma12_options;
      break;
  }

  impl->create_stream = squash_lzma_create_stream;
  impl->process_stream = squash_lzma_process_stream;
  impl->get_max_compressed_size = squash_lzma_get_max_compressed_size;

  return SQUASH_OK;
}
