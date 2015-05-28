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
#include <strings.h>

#include <squash/squash.h>

#include <bzlib.h>

enum SquashBZ2OptIndex {
  SQUASH_BZ2_OPT_LEVEL = 0,
  SQUASH_BZ2_OPT_WORK_FACTOR,
  SQUASH_BZ2_OPT_SMALL
};

static SquashOptionInfo squash_bz2_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 9 },
    .default_value.int_value = 6 },
  { "work-factor",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 250 },
    .default_value.int_value = 30 },
  { "small",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

typedef struct SquashBZ2Stream_s {
  SquashStream base_object;

  bz_stream stream;
} SquashBZ2Stream;

SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_bz2_stream_init     (SquashBZ2Stream* stream,
                                                     SquashCodec* codec,
                                                     SquashStreamType stream_type,
                                                     SquashOptions* options,
                                                     SquashDestroyNotify destroy_notify);
static SquashBZ2Stream*  squash_bz2_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void              squash_bz2_stream_destroy  (void* stream);
static void              squash_bz2_stream_free     (void* stream);

static SquashBZ2Stream*
squash_bz2_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  int bz2_e = 0;
  SquashBZ2Stream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashBZ2Stream*) malloc (sizeof (SquashBZ2Stream));
  squash_bz2_stream_init (stream, codec, stream_type, options, squash_bz2_stream_free);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    bz2_e = BZ2_bzCompressInit (&(stream->stream),
                                squash_codec_get_option_int_index (codec, options, SQUASH_BZ2_OPT_LEVEL),
                                0,
                                squash_codec_get_option_int_index (codec, options, SQUASH_BZ2_OPT_WORK_FACTOR));
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    bz2_e = BZ2_bzDecompressInit (&(stream->stream),
                                  0,
                                  squash_codec_get_option_int_index (codec, options, SQUASH_BZ2_OPT_SMALL));
  } else {
    assert (false);
  }

  if (bz2_e != BZ_OK) {
    /* We validate the params so OOM is really the only time this
       should happen, and that really shouldn't be happening here. */
    stream = squash_object_unref (stream);
  }

  return stream;
}

static void
squash_bz2_stream_init (SquashBZ2Stream* stream,
                        SquashCodec* codec,
                        SquashStreamType stream_type,
                        SquashOptions* options,
                        SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  bz_stream tmp = { 0, };
  stream->stream = tmp;
}

static void
squash_bz2_stream_destroy (void* stream) {
  switch (((SquashStream*) stream)->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      BZ2_bzCompressEnd (&(((SquashBZ2Stream*) stream)->stream));
      break;
    case SQUASH_STREAM_DECOMPRESS:
      BZ2_bzDecompressEnd (&(((SquashBZ2Stream*) stream)->stream));
      break;
  }

  squash_stream_destroy (stream);
}

static void
squash_bz2_stream_free (void* stream) {
  squash_bz2_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_bz2_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_bz2_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_bz2_status_to_squash_status (int status) {
  switch (status) {
    case BZ_OK:
    case BZ_RUN_OK:
    case BZ_FLUSH_OK:
    case BZ_FINISH_OK:
      return SQUASH_OK;
    case BZ_OUTBUFF_FULL:
      return SQUASH_BUFFER_FULL;
    case BZ_SEQUENCE_ERROR:
      return SQUASH_STATE;
    default:
      return SQUASH_FAILED;
  }
}

#define SQUASH_BZ2_STREAM_COPY_TO_BZ_STREAM(stream,bz2_stream) \
  bz2_stream->next_in = (char*) stream->next_in; \
  bz2_stream->avail_in = (unsigned int) stream->avail_in; \
  bz2_stream->next_out = (char*) stream->next_out; \
  bz2_stream->avail_out = (unsigned int) stream->avail_out

#define SQUASH_BZ2_STREAM_COPY_FROM_BZ_STREAM(stream,bz2_stream) \
  stream->next_in = (uint8_t*) bz2_stream->next_in;       \
  stream->avail_in = (size_t) bz2_stream->avail_in;            \
  stream->next_out = (uint8_t*) bz2_stream->next_out;    \
  stream->avail_out = (size_t) bz2_stream->avail_out

static SquashStatus
squash_bz2_process_stream_ex (SquashStream* stream, int action) {
  bz_stream* bz2_stream;
  int bz2_res;
  SquashStatus res;

  assert (stream != NULL);

  if (stream->avail_out == 0)
    return SQUASH_BUFFER_FULL;

  bz2_stream = &(((SquashBZ2Stream*) stream)->stream);

  SQUASH_BZ2_STREAM_COPY_TO_BZ_STREAM(stream, bz2_stream);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    bz2_res = BZ2_bzCompress (bz2_stream, action);
  } else {
    bz2_res = BZ2_bzDecompress (bz2_stream);
  }

  if (bz2_res == BZ_RUN_OK || bz2_res == BZ_OK) {
    if (bz2_stream->avail_in == 0) {
      res = SQUASH_OK;
    } else {
      res = SQUASH_PROCESSING;
    }
  } else if (bz2_res == BZ_STREAM_END) {
    res = SQUASH_END_OF_STREAM;
  } else {
    res = squash_bz2_status_to_squash_status (bz2_res);
  }

  SQUASH_BZ2_STREAM_COPY_FROM_BZ_STREAM(stream, bz2_stream);

  return res;
}

static SquashStatus
squash_bz2_finish_stream (SquashStream* stream) {
  bz_stream* bz2_stream;
  int bz2_res;
  SquashStatus res;

  if (stream->stream_type != SQUASH_STREAM_COMPRESS) {
    return squash_bz2_process_stream_ex (stream, BZ_RUN);
  }

  assert (stream != NULL);

  if (stream->avail_out == 0)
    return SQUASH_BUFFER_FULL;

  bz2_stream = &(((SquashBZ2Stream*) stream)->stream);

  SQUASH_BZ2_STREAM_COPY_TO_BZ_STREAM(stream, bz2_stream);

  bz2_res = BZ2_bzCompress (bz2_stream, BZ_FINISH);

  switch (bz2_res) {
    case BZ_FINISH_OK:
      res = SQUASH_PROCESSING;
      break;
    case BZ_STREAM_END:
      res = SQUASH_OK;
      break;
    default:
      res = squash_bz2_status_to_squash_status (bz2_res);
      break;
  }

  SQUASH_BZ2_STREAM_COPY_FROM_BZ_STREAM(stream, bz2_stream);

  return res;
}

static SquashStatus
squash_bz2_process_stream (SquashStream* stream, SquashOperation operation) {
  switch (operation) {
    case SQUASH_OPERATION_PROCESS:
      return squash_bz2_process_stream_ex (stream, BZ_RUN);
    case SQUASH_OPERATION_FLUSH:
      return squash_bz2_process_stream_ex (stream, BZ_FLUSH);
    case SQUASH_OPERATION_FINISH:
      return squash_bz2_finish_stream (stream);
  }

  assert (0);
}

static size_t
squash_bz2_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return
    uncompressed_length +
    (uncompressed_length / 100) + ((uncompressed_length % 100) > 0 ? 1 : 0) +
    600;
}

static SquashStatus
squash_bz2_decompress_buffer (SquashCodec* codec,
                              size_t* decompressed_length,
                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                              size_t compressed_length,
                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                              SquashOptions* options) {
  int small = squash_codec_get_option_bool_index (codec, options, SQUASH_BZ2_OPT_SMALL) ? 1 : 0;
  unsigned int decompressed_length_ui = (unsigned int) *decompressed_length;
  int bz2_res;

  bz2_res = BZ2_bzBuffToBuffDecompress ((char*) decompressed, &decompressed_length_ui,
                                        (char*) compressed, (unsigned int) compressed_length,
                                        small, 0);
  if (bz2_res == BZ_OK) {
    *decompressed_length = decompressed_length_ui;
  }

  return squash_bz2_status_to_squash_status (bz2_res);
}

static SquashStatus
squash_bz2_compress_buffer (SquashCodec* codec,
                            size_t* compressed_length,
                            uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOptions* options) {
  int bz2_res;
  unsigned int compressed_length_ui = (unsigned int) *compressed_length;

  bz2_res = BZ2_bzBuffToBuffCompress ((char*) compressed, &compressed_length_ui,
                                      (char*) uncompressed, (unsigned int) uncompressed_length,
                                      squash_codec_get_option_int_index (codec, options, SQUASH_BZ2_OPT_LEVEL),
                                      0,
                                      squash_codec_get_option_int_index (codec, options, SQUASH_BZ2_OPT_WORK_FACTOR));
  if (bz2_res == BZ_OK) {
    *compressed_length = compressed_length_ui;
  }

  return squash_bz2_status_to_squash_status (bz2_res);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  if (strcmp ("bzip2", squash_codec_get_name (codec)) == 0) {
    funcs->options = squash_bz2_options;
    funcs->create_stream = squash_bz2_create_stream;
    funcs->process_stream = squash_bz2_process_stream;
    funcs->get_max_compressed_size = squash_bz2_get_max_compressed_size;
    funcs->decompress_buffer = squash_bz2_decompress_buffer;
    funcs->compress_buffer = squash_bz2_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
