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

enum SquashZpaqOptIndex {
  SQUASH_ZPAQ_OPT_LEVEL = 0
};

static SquashOptionInfo squash_zpaq_options[] = {
  { (char*) "level",
    SQUASH_OPTION_TYPE_RANGE_INT, },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

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

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecImpl* impl);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_plugin   (SquashPlugin* plugin);

static void               squash_zpaq_stream_init     (SquashZpaqStream* stream,
                                                       SquashCodec* codec,
                                                       SquashStreamType stream_type,
                                                       SquashOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashZpaqStream*  squash_zpaq_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
static void               squash_zpaq_stream_destroy  (void* stream);
static void               squash_zpaq_stream_free     (void* stream);

static _Thread_local SquashStream* squash_zpaq_thread_stream = NULL;

void
libzpaq::error (const char* msg) {
  SquashStatus status = SQUASH_FAILED;
  assert (squash_zpaq_thread_stream != NULL);
  if (strcmp (msg, "Out of memory") == 0)
    status = SQUASH_MEMORY;
  squash_stream_yield (squash_zpaq_thread_stream, status);
}

SquashZpaqIO::SquashZpaqIO (SquashZpaqStream* str) {
  this->stream = str;
}

int SquashZpaqIO::get () {
  SquashStream* str = (SquashStream*) this->stream;

  while (str->avail_in == 0 && this->stream->operation == SQUASH_OPERATION_PROCESS) {
    this->stream->operation = squash_stream_yield (str, SQUASH_OK);
  }

  if (str->avail_in == 0) {
    return -1;
  } else {
    int r = str->next_in[0];
    str->next_in++;
    str->avail_in--;
    return r;
  }
}

void SquashZpaqIO::put (int c) {
  uint8_t v = (uint8_t) c;
  this->write ((const char*) &v, 1);
}

void SquashZpaqIO::write (const char* buf, int n) {
  SquashStream* str = (SquashStream*) this->stream;

  size_t written = 0;
  size_t remaining = (size_t) n;
  while (remaining > 0) {
    const size_t cp_size = (remaining < str->avail_out) ? remaining : str->avail_out;

    memcpy (str->next_out, buf + written, cp_size);

    written += cp_size;
    remaining -= cp_size;
    str->next_out += cp_size;
    str->avail_out -= cp_size;

    if (remaining != 0)
      this->stream->operation = squash_stream_yield (str, SQUASH_PROCESSING);
  }
}

static SquashZpaqStream* squash_zpaq_stream_new (SquashCodec* codec,
                                                 SquashStreamType stream_type,
                                                 SquashOptions* options) {
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
                         SquashOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, options, destroy_notify);

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
  return (SquashStream*) squash_zpaq_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_zpaq_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashZpaqStream* s = (SquashZpaqStream*) stream;

  s->operation = operation;

  try {
    if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
      char level_s[2];
      snprintf (level_s, sizeof(level_s), "%d", squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_ZPAQ_OPT_LEVEL));

      squash_zpaq_thread_stream = stream;
      compress (s->stream, s->stream, level_s);
      squash_zpaq_thread_stream = NULL;
    } else {
      squash_zpaq_thread_stream = stream;
      decompress (s->stream, s->stream);
      squash_zpaq_thread_stream = NULL;
    }
  } catch (const std::bad_alloc& e) {
    return SQUASH_MEMORY;
  } catch (...) {
    return SQUASH_FAILED;
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
squash_plugin_init_plugin (SquashPlugin* plugin) {
  const SquashOptionInfoRangeInt level_range = { 1, 5, 0, false };

  squash_zpaq_options[SQUASH_ZPAQ_OPT_LEVEL].default_value.int_value = 1;
  squash_zpaq_options[SQUASH_ZPAQ_OPT_LEVEL].info.range_int = level_range;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zpaq", name) == 0) {
    impl->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    impl->options = squash_zpaq_options;
    impl->create_stream = squash_zpaq_create_stream;
    impl->process_stream = squash_zpaq_process_stream;
    impl->get_max_compressed_size = squash_zpaq_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
