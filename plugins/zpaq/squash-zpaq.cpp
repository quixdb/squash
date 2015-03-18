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

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
 #if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
  #define _Thread_local __thread
 #else
  #define _Thread_local __declspec(thread)
 #endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
 #define _Thread_local __thread
#endif

#include "libzpaq.h"

#define SQUASH_ZPAQ_DEFAULT_LEVEL 1

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct _SquashZpaqOptions {
  SquashOptions base_object;

  int level;
} SquashZpaqOptions;

typedef struct _SquashZpaqStream SquashZpaqStream;

class SquashZpaqIO: public libzpaq::Reader, public libzpaq::Writer {
public:
  SquashZpaqStream* stream;

  int get ();
  void put (int c);

  void write (const char* buf, int n);

  SquashZpaqIO(SquashZpaqStream* stream);
};

struct _SquashZpaqStream {
  SquashStream base_object;

  SquashZpaqIO* stream;

  SquashOperation operation;
};

extern "C" SquashStatus   squash_plugin_init_codec    (SquashCodec* codec, SquashCodecFuncs* funcs);

static void               squash_zpaq_options_init    (SquashZpaqOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashZpaqOptions* squash_zpaq_options_new     (SquashCodec* codec);
static void               squash_zpaq_options_destroy (void* options);
static void               squash_zpaq_options_free    (void* options);

static void               squash_zpaq_stream_init     (SquashZpaqStream* stream,
                                                       SquashCodec* codec,
                                                       SquashStreamType stream_type,
                                                       SquashZpaqOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashZpaqStream*  squash_zpaq_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashZpaqOptions* options);
static void               squash_zpaq_stream_destroy  (void* stream);
static void               squash_zpaq_stream_free     (void* stream);

static _Thread_local SquashStream* squash_zpaq_thread_stream = NULL;

void
libzpaq::error (const char* msg) {
  assert (squash_zpaq_thread_stream != NULL);
  squash_stream_yield (squash_zpaq_thread_stream, SQUASH_FAILED);
}

SquashZpaqIO::SquashZpaqIO (SquashZpaqStream* stream) {
  this->stream = stream;
}

int SquashZpaqIO::get () {
  SquashStream* stream = (SquashStream*) this->stream;

  while (stream->avail_in == 0 && this->stream->operation == SQUASH_OPERATION_PROCESS) {
    this->stream->operation = squash_stream_yield (stream, SQUASH_OK);
  }

  if (stream->avail_in == 0) {
    return -1;
  } else {
    int r = stream->next_in[0];
    stream->next_in++;
    stream->avail_in--;
    return r;
  }
}

void SquashZpaqIO::put (int c) {
  uint8_t v = (uint8_t) c;
  this->write ((const char*) &v, 1);
}

void SquashZpaqIO::write (const char* buf, int n) {
  SquashStream* stream = (SquashStream*) this->stream;

  size_t written = 0;
  size_t remaining = (size_t) n;
  while (remaining > 0) {
    const size_t cp_size = (remaining < stream->avail_out) ? remaining : stream->avail_out;

    memcpy (stream->next_out, buf + written, cp_size);

    written += cp_size;
    remaining -= cp_size;
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;

    if (remaining != 0)
      this->stream->operation = squash_stream_yield (stream, SQUASH_PROCESSING);
  }
}

static void
squash_zpaq_options_init (SquashZpaqOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

	options->level = SQUASH_ZPAQ_DEFAULT_LEVEL;
}

static SquashZpaqOptions*
squash_zpaq_options_new (SquashCodec* codec) {
  SquashZpaqOptions* options;

  options = (SquashZpaqOptions*) malloc (sizeof (SquashZpaqOptions));
  squash_zpaq_options_init (options, codec, squash_zpaq_options_free);

  return options;
}

static void
squash_zpaq_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_zpaq_options_free (void* options) {
  squash_zpaq_options_destroy ((SquashZpaqOptions*) options);
  free (options);
}

static SquashOptions*
squash_zpaq_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_zpaq_options_new (codec);
}

static SquashStatus
squash_zpaq_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashZpaqOptions* opts = (SquashZpaqOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 5 ) {
      opts->level = level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static SquashZpaqStream* squash_zpaq_stream_new (SquashCodec* codec,
                                                 SquashStreamType stream_type,
                                                 SquashZpaqOptions* options) {
  SquashZpaqStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashZpaqStream*) malloc (sizeof (SquashZpaqStream));
  squash_zpaq_stream_init (stream, codec, stream_type, options, squash_zpaq_stream_free);

  return stream;
}

static void
squash_zpaq_stream_init (SquashZpaqStream* stream,
                         SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashZpaqOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  stream->stream = new SquashZpaqIO(stream);
  stream->stream->stream = stream;
}

static void
squash_zpaq_stream_destroy (void* stream) {
  delete ((SquashZpaqStream*) stream)->stream;

  squash_stream_destroy (stream);
}

static void
squash_zpaq_stream_free (void* stream) {
  squash_zpaq_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_zpaq_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_zpaq_stream_new (codec, stream_type, (SquashZpaqOptions*) options);
}

static SquashStatus
squash_zpaq_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashZpaqStream* s = (SquashZpaqStream*) stream;

  s->operation = operation;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    char level_s[2];
    snprintf (level_s, sizeof(level_s), "%d", (stream->options != NULL) ? ((SquashZpaqOptions*) stream->options)->level : SQUASH_ZPAQ_DEFAULT_LEVEL);

    squash_zpaq_thread_stream = stream;
    compress (s->stream, s->stream, level_s);
    squash_zpaq_thread_stream = NULL;
  } else {
    squash_zpaq_thread_stream = stream;
    decompress (s->stream, s->stream);
    squash_zpaq_thread_stream = NULL;
  }

  return SQUASH_OK;
}

static size_t
squash_zpaq_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return
    uncompressed_length +
    ((uncompressed_length / 100) * 1) + ((uncompressed_length % 100) > 0 ? 1 : 0) +
    377;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zpaq", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    funcs->create_options = squash_zpaq_create_options;
    funcs->parse_option = squash_zpaq_parse_option;
    funcs->create_stream = squash_zpaq_create_stream;
    funcs->process_stream = squash_zpaq_process_stream;
    funcs->get_max_compressed_size = squash_zpaq_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
