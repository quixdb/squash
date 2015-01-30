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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <squash/squash.h>

#include "libzling.h"

#define SQUASH_ZLING_DEFAULT_LEVEL 0

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct _SquashZlingOptions {
  SquashOptions base_object;

  int level;
} SquashZlingOptions;

typedef struct _SquashZlingStream SquashZlingStream;

struct SquashZlingIO: public baidu::zling::Inputter, public baidu::zling::Outputter {
  SquashZlingIO (SquashZlingStream* s):
    stream(s) {}

  size_t GetData(unsigned char* buf, size_t len);
  size_t PutData(unsigned char* buf, size_t len);
  bool   IsEnd();
  bool   IsErr();

private:
  SquashZlingStream* stream;
};

struct _SquashZlingStream {
  SquashStream base_object;

  SquashZlingIO* stream;

  SquashOperation operation;
};

extern "C" SquashStatus    squash_plugin_init_codec     (SquashCodec* codec, SquashCodecFuncs* funcs);

static void                squash_zling_options_init    (SquashZlingOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashZlingOptions* squash_zling_options_new     (SquashCodec* codec);
static void                squash_zling_options_destroy (void* options);
static void                squash_zling_options_free    (void* options);

static void                squash_zling_stream_init     (SquashZlingStream* stream,
                                                         SquashCodec* codec,
                                                         SquashStreamType stream_type,
                                                         SquashZlingOptions* options,
                                                         SquashDestroyNotify destroy_notify);
static SquashZlingStream*  squash_zling_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashZlingOptions* options);
static void                squash_zling_stream_destroy  (void* stream);
static void                squash_zling_stream_free     (void* stream);

size_t
SquashZlingIO::GetData (unsigned char* buf, size_t len) {
  SquashStream* stream = (SquashStream*) this->stream;

  while (stream->avail_in == 0 && this->stream->operation == SQUASH_OPERATION_PROCESS) {
    this->stream->operation = squash_stream_yield (stream, SQUASH_OK);
  }

  if (stream->avail_in == 0) {
    return -1;
  } else {
    const size_t cp_size = stream->avail_in < len ? stream->avail_in : len;

    memcpy (buf, stream->next_in, cp_size);
    stream->next_in += cp_size;
    stream->avail_in -= cp_size;

    return cp_size;
  }
}

size_t
SquashZlingIO::PutData(unsigned char* buf, size_t len) {
  SquashStream* stream = (SquashStream*) this->stream;

  size_t written = 0;
  size_t remaining = len;
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

  return written;
}

bool
SquashZlingIO::IsEnd() {
  SquashStream* stream = (SquashStream*) this->stream;

  while (stream->avail_in == 0 && this->stream->operation == SQUASH_OPERATION_PROCESS) {
    this->stream->operation = squash_stream_yield (stream, SQUASH_OK);
  }

  return (this->stream->operation == SQUASH_OPERATION_FINISH && stream->avail_in == 0);
}

bool
SquashZlingIO::IsErr() {
  return false;
}

static void
squash_zling_options_init (SquashZlingOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

	options->level = SQUASH_ZLING_DEFAULT_LEVEL;
}

static SquashZlingOptions*
squash_zling_options_new (SquashCodec* codec) {
  SquashZlingOptions* options;

  options = (SquashZlingOptions*) malloc (sizeof (SquashZlingOptions));
  squash_zling_options_init (options, codec, squash_zling_options_free);

  return options;
}

static void
squash_zling_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_zling_options_free (void* options) {
  squash_zling_options_destroy ((SquashZlingOptions*) options);
  free (options);
}

static SquashOptions*
squash_zling_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_zling_options_new (codec);
}

static SquashStatus
squash_zling_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashZlingOptions* opts = (SquashZlingOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 0 && level <= 4 ) {
      opts->level = level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static SquashZlingStream* squash_zling_stream_new (SquashCodec* codec,
                                                 SquashStreamType stream_type,
                                                 SquashZlingOptions* options) {
  SquashZlingStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashZlingStream*) malloc (sizeof (SquashZlingStream));
  squash_zling_stream_init (stream, codec, stream_type, options, squash_zling_stream_free);

  return stream;
}

static void
squash_zling_stream_init (SquashZlingStream* stream,
                         SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashZlingOptions* options,
                         SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  stream->stream = new SquashZlingIO(stream);
}

static void
squash_zling_stream_destroy (void* stream) {
  delete ((SquashZlingStream*) stream)->stream;

  squash_stream_destroy (stream);
}

static void
squash_zling_stream_free (void* stream) {
  squash_zling_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_zling_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_zling_stream_new (codec, stream_type, (SquashZlingOptions*) options);
}

static SquashStatus
squash_zling_thread_process (SquashStream* stream, SquashOperation operation) {
  SquashStatus res = SQUASH_OK;
  SquashZlingStream* s = (SquashZlingStream*) stream;

  s->operation = operation;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    int level = SQUASH_ZLING_DEFAULT_LEVEL;
    if (stream->options != NULL)
      level = ((SquashZlingOptions*) stream->options)->level;

    res = baidu::zling::Encode(s->stream, s->stream, NULL, level) == 0 ?
      SQUASH_OK : SQUASH_FAILED;
  } else {
    res = baidu::zling::Decode(s->stream, s->stream, NULL) == 0 ?
      SQUASH_OK : SQUASH_FAILED;
  }

  return res;
}

static size_t
squash_zling_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return
    uncompressed_length + 288 + (uncompressed_length / 8);
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zling", name) == 0) {
    funcs->create_options = squash_zling_create_options;
    funcs->parse_option = squash_zling_parse_option;
    funcs->create_stream = squash_zling_create_stream;
    funcs->thread_process = squash_zling_thread_process;
    funcs->get_max_compressed_size = squash_zling_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
