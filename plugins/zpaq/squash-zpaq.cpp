/* Copyright (c) 2013 The Squash Authors
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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include <squash/squash.h>

#include "libzpaq.h"

#define SQUASH_ZPAQ_DEFAULT_LEVEL 2

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct _SquashZpaqOptions {
  SquashOptions base_object;

  int level;
} SquashZpaqOptions;

typedef struct _SquashZpaqStream SquashZpaqStream;
class SquashZpaqStreamImpl;
class SquashZpaqCompressor;
class SquashZpaqDecompresser;

class SquashZpaqStreamImpl: public libzpaq::Reader, public libzpaq::Writer {
public:
  SquashZpaqStream* stream;

  size_t shuffle_bytes ();

  int get ();
  void put (int c);

  virtual SquashStatus process () = 0;
  virtual SquashStatus finish () = 0;

  void init (SquashCodec* codec, SquashStreamType stream_type, SquashZpaqOptions* options);
  ~SquashZpaqStreamImpl ();

protected:
  bool is_initialized;
  bool is_finished;

  uint8_t* buffer;
  off_t buffer_pos;
  size_t buffer_length;
  size_t buffer_allocated;
};

class SquashZpaqCompressor: public SquashZpaqStreamImpl {
public:
  SquashZpaqCompressor (SquashCodec* codec, SquashZpaqOptions* options);
  libzpaq::Compressor compressor;

  SquashStatus process ();
  SquashStatus finish ();
};

class SquashZpaqDecompresser: public SquashZpaqStreamImpl {
public:
  SquashZpaqDecompresser (SquashCodec* codec, SquashZpaqOptions* options);
  libzpaq::Decompresser decompresser;

  SquashStatus process ();
  SquashStatus finish ();
};

struct _SquashZpaqStream {
  SquashStream base_object;

  SquashZpaqStreamImpl* stream;
};

#define SQUASH_ZPAQ_STREAM(type, stream) ((type*) (((SquashZpaqStream*) stream)->stream))

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
static void               squash_zpaq_stream_destroy (void* stream);
static void               squash_zpaq_stream_free    (void* stream);

static pthread_key_t squash_zpaq_local_error;
static pthread_once_t squash_zpaq_local_error_once = PTHREAD_ONCE_INIT;

static inline const char*
squash_zpaq_get_error () {
  const char* msg = (const char*) pthread_getspecific (squash_zpaq_local_error);
  if (msg != NULL) {
    pthread_setspecific (squash_zpaq_local_error, NULL);
    abort ();
  }
  return msg;
}

void
libzpaq::error (const char* msg) {
  pthread_setspecific (squash_zpaq_local_error, msg);
}

void
SquashZpaqStreamImpl::init (SquashCodec* codec, SquashStreamType stream_type, SquashZpaqOptions* options) {
  stream = (SquashZpaqStream*) malloc (sizeof (SquashZpaqStream));
  stream->stream = this;
  squash_zpaq_stream_init (stream, codec, stream_type, options, squash_zpaq_stream_free);

  /* Wait to intialize the Compressor/Decompresser until we really
     need it, so we are more likely to be able to write directly to a
     user-supplied buffer instead of creating our own. */
  is_initialized = false;
  is_finished = false;

  buffer = NULL;
  buffer_pos = 0;
  buffer_length = 0;
  buffer_allocated = 0;
}

SquashZpaqStreamImpl::~SquashZpaqStreamImpl () {
  if (buffer != NULL) {
    free (buffer);
  }

  free (stream);
}

int
SquashZpaqStreamImpl::get () {
  int ret = -1;
  if (stream->base_object.avail_in > 0) {
    ret = *(stream->base_object.next_in++) & 0xff;
    stream->base_object.avail_in--;
  }
  return ret;
}

void
SquashZpaqStreamImpl::put(int c) {
  if (buffer_length == 0 && stream->base_object.avail_out > 0) {
    *(stream->base_object.next_out++) = (uint8_t) c & 0xff;
    stream->base_object.avail_out--;
  } else {
    if (buffer == NULL) {
      buffer = (uint8_t*) malloc (buffer_allocated = 1 << 16);
      memset (buffer, 0, buffer_allocated);
    } else if ((buffer_pos + buffer_length) == buffer_allocated) {
      buffer = (uint8_t*) realloc (buffer, buffer_allocated <<= 1);
    }

    buffer[buffer_pos + buffer_length++] = (uint8_t) (c & 0xff);
  }
}

size_t
SquashZpaqStreamImpl::shuffle_bytes () {
  size_t copy_size = 0;

  if (buffer_length > 0) {
    copy_size = MIN(stream->base_object.avail_out, buffer_length);

    memcpy (stream->base_object.next_out, buffer + buffer_pos, copy_size);
    stream->base_object.avail_out -= copy_size;
    stream->base_object.next_out += copy_size;

    buffer_length -= copy_size;
    if (buffer_length == 0) {
      buffer_pos = 0;
    } else {
      buffer_pos += copy_size;
    }
  }

  return copy_size;
}

SquashZpaqCompressor::SquashZpaqCompressor (SquashCodec* codec, SquashZpaqOptions* options) {
  init (codec, SQUASH_STREAM_COMPRESS, options);

  compressor.setInput (this);
  compressor.setOutput (this);
}

SquashStatus
SquashZpaqCompressor::process () {
  if (buffer_length > 0) {
    if (shuffle_bytes () > 0) {
      if (buffer_length > 0 || stream->base_object.avail_out == 0) {
        return SQUASH_PROCESSING;
      }
    } else {
      return SQUASH_BUFFER_FULL;
    }
  }

  if (!is_initialized) {
    compressor.startBlock ((stream->base_object.options != NULL) ? ((SquashZpaqOptions*) (stream->base_object.options))->level : SQUASH_ZPAQ_DEFAULT_LEVEL);
    compressor.startSegment ();

    is_initialized = true;
  }

  while (stream->base_object.avail_out > 0 && stream->base_object.avail_in > 0) {
    compressor.compress (1);
  }

  return ((stream->base_object.avail_in == 0) && (buffer_length == 0)) ? SQUASH_OK : SQUASH_PROCESSING;
}

SquashStatus
SquashZpaqCompressor::finish () {
  if (stream->base_object.avail_in > 0) {
    SquashStatus res = process ();

    if (res != SQUASH_OK) {
      return res;
    }
  }

  if (!is_finished) {
    compressor.endSegment ();
    compressor.endBlock ();

    is_finished = true;
  }

  return buffer_length == 0 ? SQUASH_OK : SQUASH_PROCESSING;
}

SquashZpaqDecompresser::SquashZpaqDecompresser (SquashCodec* codec, SquashZpaqOptions* options) {
  init (codec, SQUASH_STREAM_DECOMPRESS, options);

  decompresser.setInput (this);
  decompresser.setOutput (this);
}

SquashStatus
SquashZpaqDecompresser::process () {
  bool progress = false;
  uint8_t* orig_next_out = stream->base_object.next_out;

  if (buffer_length > 0) {
    if (shuffle_bytes () > 0) {
      if (buffer_length > 0 || stream->base_object.avail_out == 0) {
        return SQUASH_PROCESSING;
      }
      progress = true;
    } else {
      return SQUASH_BUFFER_FULL;
    }
  }

  if (!is_initialized) {
    if (!decompresser.findBlock ()) {
      return SQUASH_FAILED;
    }
    if (!decompresser.findFilename ()) {
      return SQUASH_FAILED;
    }
    decompresser.readComment ();

    is_initialized = true;
    progress = true;
  }

  while (stream->base_object.avail_out > 0 && stream->base_object.avail_in > 0 && !is_finished) {
    progress = true;
    if (!decompresser.decompress (1)) {
      decompresser.readSegmentEnd (); 
      is_finished = true;
      break;
    }
    if (squash_zpaq_get_error () != NULL) {
      return SQUASH_FAILED;
    }
  }

  if (is_finished) {
    return buffer_length == 0 ? SQUASH_END_OF_STREAM : SQUASH_PROCESSING;
  } else {
    return stream->base_object.avail_in == 0 ? SQUASH_OK : SQUASH_PROCESSING;
  }
}

SquashStatus
SquashZpaqDecompresser::finish () {
  SquashStatus res = SQUASH_BUFFER_EMPTY;

  if (stream->base_object.avail_in > 0) {
    SquashStatus res = process ();

    switch (res) {
      case SQUASH_END_OF_STREAM:
        return SQUASH_OK;
      case SQUASH_OK: /* Input data is incomplete */
        return SQUASH_FAILED;
      case SQUASH_BUFFER_FULL:
        break;
      case SQUASH_PROCESSING:
      default:
        return res;
    }
  }

  /* We should only hit this if we have consumed all the data available. */
  if (buffer_length > 0) {
    return SQUASH_PROCESSING;
  } else if (is_finished) {
    return SQUASH_OK;
  } else {
    /* Try to consume one more byte, even if it means using the buffer, so we can return SQUASH_END_OF_STREAM. */
    if (!decompresser.decompress (1)) {
      decompresser.readSegmentEnd (); 
      is_finished = true;
      return SQUASH_END_OF_STREAM;
    } else {
      return SQUASH_FAILED;
    }
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
    if ( *endptr == '\0' && level >= 1 && level <= 3 ) {
      opts->level = level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static void
squash_zpaq_stream_init (SquashZpaqStream* stream,
                         SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashZpaqOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init (stream, codec, stream_type, (SquashOptions*) options, squash_zpaq_stream_free);
}

static void
squash_zpaq_stream_destroy (void* stream) {
  squash_stream_destroy (stream);
}

static void
squash_zpaq_stream_free (void* stream) {
  SquashZpaqStreamImpl* zps = ((SquashZpaqStream*) stream)->stream;

  squash_zpaq_stream_destroy (stream);

  delete zps;
}

static SquashStream*
squash_zpaq_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashZpaqStreamImpl* stream;
  if (stream_type == SQUASH_STREAM_COMPRESS) {
    stream = new SquashZpaqCompressor (codec, (SquashZpaqOptions*) options);
  } else {
    stream = new SquashZpaqDecompresser (codec, (SquashZpaqOptions*) options);
  }

  return (SquashStream*) (stream->stream);
}

static SquashStatus
squash_zpaq_process_stream (SquashStream* stream) {
  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    return SQUASH_ZPAQ_STREAM(SquashZpaqCompressor, stream)->process ();
  } else {
    return SQUASH_ZPAQ_STREAM(SquashZpaqDecompresser, stream)->process ();
  }
}

static SquashStatus
squash_zpaq_finish_stream (SquashStream* stream) {
  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    return SQUASH_ZPAQ_STREAM(SquashZpaqCompressor, stream)->finish ();
  } else {
    return SQUASH_ZPAQ_STREAM(SquashZpaqDecompresser, stream)->finish ();
  }
}

static size_t
squash_zpaq_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  /* 37 byte header + 10 byte footer + 101% data.  The header and
     footer should be okay, but the extra 1% is just based on some
     quick tests with random data.  If this isn't sufficient *please*
     file a bug so we can bump it. */
  return
    uncompressed_length +
    ((uncompressed_length / 100) * 1) + ((uncompressed_length % 100) > 0 ? 1 : 0) +
    47;
}

static void
squash_zpaq_local_error_create () {
  pthread_key_create (&squash_zpaq_local_error, NULL);
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zpaq", name) == 0) {
    funcs->create_options = squash_zpaq_create_options;
    funcs->parse_option = squash_zpaq_parse_option;
    funcs->create_stream = squash_zpaq_create_stream;
    funcs->process_stream = squash_zpaq_process_stream;
    funcs->finish_stream = squash_zpaq_finish_stream;
    funcs->get_max_compressed_size = squash_zpaq_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  pthread_once (&squash_zpaq_local_error_once, squash_zpaq_local_error_create);

  return SQUASH_OK;
}
