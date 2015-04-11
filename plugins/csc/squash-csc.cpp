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

typedef struct _SquashCscOptions {
  SquashOptions base_object;

  int level;
  uint32_t dict_size;
  bool enable_delta;
  bool enable_exe;
  bool enable_txt;
} SquashCscOptions;

#define SQUASH_CSC_DEFAULT_DICT_SIZE (1024 * 1024 * 64)
#define SQUASH_CSC_DEFAULT_LEVEL 2

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

extern "C" SquashStatus  squash_plugin_init_codec   (SquashCodec* codec, SquashCodecFuncs* funcs);

static void              squash_csc_options_init    (SquashCscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashCscOptions* squash_csc_options_new     (SquashCodec* codec);
static void              squash_csc_options_destroy (void* options);
static void              squash_csc_options_free    (void* options);

static void              squash_csc_stream_init     (SquashCscStream* stream,
                                                     SquashCodec* codec,
                                                     SquashStreamType stream_type,
                                                     SquashCscOptions* options,
                                                     SquashDestroyNotify destroy_notify);
static SquashCscStream*  squash_csc_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashCscOptions* options);
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
      if (s->operation == SQUASH_OPERATION_FINISH)
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

    if (remaining != 0)
      s->operation = squash_stream_yield ((SquashStream*) stream, SQUASH_PROCESSING);
  }

  assert (remaining == 0);
  return (size - remaining);
}

static void
squash_csc_options_init (SquashCscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

	options->level = SQUASH_CSC_DEFAULT_LEVEL;
  options->dict_size = SQUASH_CSC_DEFAULT_DICT_SIZE;
}

static SquashCscOptions*
squash_csc_options_new (SquashCodec* codec) {
  SquashCscOptions* options;

  options = (SquashCscOptions*) malloc (sizeof (SquashCscOptions));
  squash_csc_options_init (options, codec, squash_csc_options_free);

  return options;
}

static void
squash_csc_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_csc_options_free (void* options) {
  squash_csc_options_destroy ((SquashCscOptions*) options);
  free (options);
}

static SquashOptions*
squash_csc_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_csc_options_new (codec);
}

static bool
string_to_bool (const char* value, bool* result) {
  if (strcasecmp (value, "true") == 0) {
    *result = true;
  } else if (strcasecmp (value, "false")) {
    *result = false;
  } else {
    return false;
  }
  return true;
}

static SquashStatus
squash_csc_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashCscOptions* opts = (SquashCscOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 5 ) {
      opts->level = level;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "dict-size") == 0) {
    const unsigned long dict_size = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && dict_size >= 32768 && dict_size <= 1073741824 ) {
      opts->dict_size = (uint32_t) dict_size;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "delta-filter") == 0) {
    bool res;
    if (string_to_bool(value, &res)) {
      opts->enable_delta = res;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "exe-filter") == 0) {
    bool res;
    if (string_to_bool(value, &res)) {
      opts->enable_exe = res;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else if (strcasecmp (key, "txt-filter") == 0) {
    bool res;
    if (string_to_bool(value, &res)) {
      opts->enable_txt = res;
    } else {
      return squash_error (SQUASH_BAD_VALUE);
    }
  } else {
    return squash_error (SQUASH_BAD_PARAM);
  }

  return SQUASH_OK;
}

static void
squash_csc_stream_init (SquashCscStream* s,
                        SquashCodec* codec,
                        SquashStreamType stream_type,
                        SquashCscOptions* options,
                        SquashDestroyNotify destroy_notify) {
  SquashStream* stream = (SquashStream*) s;

  squash_stream_init (stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    s->ctx.comp = NULL;
  } else {
    s->ctx.decomp = NULL;
  }
}

static SquashCscStream* squash_csc_stream_new (SquashCodec* codec,
                                               SquashStreamType stream_type,
                                               SquashCscOptions* options) {
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
  return (SquashStream*) squash_csc_stream_new (codec, stream_type, (SquashCscOptions*) options);
}

static SquashStatus
squash_csc_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashCscStream* s = (SquashCscStream*) stream;

  s->operation = operation;

  CSCProps props;
  unsigned char props_buf[CSC_PROP_SIZE];

  struct SquashCscInStream istream = { squash_csc_reader, s };
  struct SquashCscOutStream ostream = { squash_csc_writer, s };

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    if (stream->options != NULL) {
      SquashCscOptions* opts = (SquashCscOptions*) stream->options;
      CSCEncProps_Init (&props, opts->dict_size, opts->level);
      props.DLTFilter = opts->enable_delta;
      props.EXEFilter = opts->enable_exe;
      props.TXTFilter = opts->enable_txt;
    } else {
      CSCEncProps_Init (&props, SQUASH_CSC_DEFAULT_DICT_SIZE, SQUASH_CSC_DEFAULT_LEVEL);
    }

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
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("csc", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    funcs->create_options = squash_csc_create_options;
    funcs->parse_option = squash_csc_parse_option;
    funcs->create_stream = squash_csc_create_stream;
    funcs->process_stream = squash_csc_process_stream;
    funcs->get_max_compressed_size = squash_csc_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
