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

#include <squash/squash.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <csc_enc.h>
#include <csc_dec.h>
#include <Types.h>

enum SquashCscOptionIndex {
  SQUASH_CSC_OPT_LEVEL = 0,
  SQUASH_CSC_OPT_DICT_SIZE,
  SQUASH_CSC_OPT_DELTA_FILTER,
  SQUASH_CSC_OPT_EXE_FILTER,
  SQUASH_CSC_OPT_TXT_FILTER,
};

/* C++ doesn't allow us to initialize this correctly here (or, at
   least, I can't figure out how to do it), so there is some extra
   code in the init_plugin func to finish it off. */
static SquashOptionInfo squash_csc_options[] = {
  { .name = (char*) "level",
    .type = SQUASH_OPTION_TYPE_RANGE_INT },
  { .name = (char*) "dict-size",
    .type = SQUASH_OPTION_TYPE_RANGE_SIZE },
  { .name = (char*) "delta-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { .name = (char*) "exe-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { .name = (char*) "txt-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

typedef size_t (*SquashCscISeqOutStreamFunc)(void *p, const void *buf, size_t size);
typedef SRes (*SquashCscISeqInStreamFunc)(void *p, void *buf, size_t *size);

typedef struct SquashCscStream_s {
  SquashStream base_object;

  SquashOperation operation;

  union {
    CSCEncHandle comp;
    CSCDecHandle decomp;
  } ctx;
} SquashCscStream;

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_plugin  (SquashPlugin* plugin);

static void              squash_csc_stream_init     (SquashCscStream* stream,
                                                     SquashCodec* codec,
                                                     SquashStreamType stream_type,
                                                     SquashOptions* options,
                                                     SquashDestroyNotify destroy_notify);
static SquashCscStream*  squash_csc_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void              squash_csc_stream_destroy  (void* stream);
static void              squash_csc_stream_free     (void* stream);

struct SquashCscInStream {
  ISeqInStream is;
  SquashCscStream* s;
};

struct SquashCscOutStream {
  ISeqOutStream os;
  SquashCscStream* s;
};

static SRes
squash_csc_reader (void* istream, void* buf, size_t* size) {
  SquashCscStream* s = (SquashCscStream*) ((struct SquashCscInStream*) istream)->s;
  SquashStream* stream = (SquashStream*) s;
  uint8_t* buffer = (uint8_t*) buf;

  size_t remaining = *size;
  while (remaining != 0) {
    const size_t cp_size = (stream->avail_in < remaining) ? stream->avail_in : remaining;

    if (cp_size > 0) {
      memcpy (buffer + (*size - remaining), stream->next_in, cp_size);
      stream->next_in += cp_size;
      stream->avail_in -= cp_size;
      remaining -= cp_size;
    }

    if (remaining != 0) {
      if (s->operation == SQUASH_OPERATION_FINISH || s->operation == SQUASH_OPERATION_TERMINATE)
        break;

      s->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_OK);
    }
  }

  *size = (*size - remaining);
  return 0;
}

static size_t
squash_csc_writer (void* ostream, const void* buf, size_t size) {
  SquashCscStream* s = (SquashCscStream*) ((struct SquashCscOutStream*) ostream)->s;
  SquashStream* stream = (SquashStream*) s;
  const uint8_t* buffer = (uint8_t*) buf;

  assert (size != 0);
  size_t remaining = size;
  while (remaining != 0) {
    const size_t cp_size = (stream->avail_out < remaining) ? stream->avail_out : remaining;

    if (cp_size != 0) {
      memcpy (stream->next_out, buffer + (size - remaining), cp_size);
      stream->next_out += cp_size;
      stream->avail_out -= cp_size;
      remaining -= cp_size;
    }

    if (remaining != 0) {
      if (s->operation == SQUASH_OPERATION_TERMINATE)
        break;
      s->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_PROCESSING);
    }
  }

  return (size - remaining);
}

static void
squash_csc_stream_init (SquashCscStream* s,
                        SquashCodec* codec,
                        SquashStreamType stream_type,
                        SquashOptions* options,
                        SquashDestroyNotify destroy_notify) {
  SquashStream* stream = (SquashStream*) s;

  squash_stream_init (stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    s->ctx.comp = NULL;
  } else {
    s->ctx.decomp = NULL;
  }
}

static SquashCscStream*
squash_csc_stream_new (SquashCodec* codec,
                       SquashStreamType stream_type,
                       SquashOptions* options) {
  SquashCscStream* stream;

  stream = (SquashCscStream*) malloc (sizeof (SquashCscStream));
  squash_csc_stream_init (stream, codec, stream_type, options, squash_csc_stream_free);

  return stream;
}

static void
squash_csc_stream_destroy (void* str) {
  SquashStream* stream = (SquashStream*) str;
  SquashCscStream* s = (SquashCscStream*) str;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    if (s->ctx.comp != NULL)
      CSCEnc_Destroy (s->ctx.comp);
  } else {
    if (s->ctx.decomp != NULL)
      CSCDec_Destroy (s->ctx.decomp);
  }

  squash_stream_destroy (stream);
}

static void
squash_csc_stream_free (void* stream) {
  squash_csc_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_csc_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_csc_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_csc_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashCscStream* s = (SquashCscStream*) stream;

  s->operation = operation;

  CSCProps props;
  unsigned char props_buf[CSC_PROP_SIZE];

  struct SquashCscInStream istream = { { squash_csc_reader }, s };
  struct SquashCscOutStream ostream = { { squash_csc_writer }, s };

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    CSCEncProps_Init (&props,
                      squash_codec_get_option_size_index (stream->codec, stream->options, SQUASH_CSC_OPT_DICT_SIZE),
                      squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_CSC_OPT_LEVEL));
    props.DLTFilter = squash_codec_get_option_bool_index (stream->codec, stream->options, SQUASH_CSC_OPT_DELTA_FILTER);
    props.EXEFilter = squash_codec_get_option_bool_index (stream->codec, stream->options, SQUASH_CSC_OPT_EXE_FILTER);
    props.TXTFilter = squash_codec_get_option_bool_index (stream->codec, stream->options, SQUASH_CSC_OPT_TXT_FILTER);

    CSCEnc_WriteProperties (&props, props_buf, 0);
    size_t bytes_written = squash_csc_writer(&ostream, props_buf, CSC_PROP_SIZE);
    if (bytes_written != CSC_PROP_SIZE)
      return squash_error (SQUASH_FAILED);

    s->ctx.comp = CSCEnc_Create (&props, (ISeqOutStream*) &ostream);
    CSCEnc_Encode (s->ctx.comp, (ISeqInStream*) &istream, NULL);
    CSCEnc_Encode_Flush (s->ctx.comp);
  } else {
    size_t prop_l = CSC_PROP_SIZE;
    squash_csc_reader (&istream, props_buf, &prop_l);
    if (prop_l != CSC_PROP_SIZE)
      return squash_error (SQUASH_FAILED);

    CSCDec_ReadProperties (&props, props_buf);

    s->ctx.decomp = CSCDec_Create (&props, (ISeqInStream*) &istream);
    CSCDec_Decode (s->ctx.decomp, (ISeqOutStream*) &ostream, NULL);
  }

  return SQUASH_OK;
}

static size_t
squash_csc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  // TODO: this could probably be improved
  return
    uncompressed_length + 64 + (uncompressed_length / 128);
}

extern "C" SquashStatus
squash_plugin_init_plugin (SquashPlugin* plugin) {
  const SquashOptionInfoRangeInt level_range = { 1, 5, 0, false };
  const SquashOptionInfoRangeSize dict_size_range = { 32768, 1073741824, 0, false };

  squash_csc_options[SQUASH_CSC_OPT_LEVEL].default_value.int_value = 2;
  squash_csc_options[SQUASH_CSC_OPT_LEVEL].info.range_int = level_range;
  squash_csc_options[SQUASH_CSC_OPT_DICT_SIZE].default_value.size_value = (1024 * 1024 * 64);
  squash_csc_options[SQUASH_CSC_OPT_DICT_SIZE].info.range_size = dict_size_range;
  squash_csc_options[SQUASH_CSC_OPT_DELTA_FILTER].default_value.bool_value = false;
  squash_csc_options[SQUASH_CSC_OPT_EXE_FILTER].default_value.bool_value = true;
  squash_csc_options[SQUASH_CSC_OPT_TXT_FILTER].default_value.bool_value = true;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("csc", name) == 0) {
    impl->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    impl->options = squash_csc_options;
    impl->create_stream = squash_csc_create_stream;
    impl->process_stream = squash_csc_process_stream;
    impl->get_max_compressed_size = squash_csc_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
