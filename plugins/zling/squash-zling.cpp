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

#include "libzling.h"

#define SQUASH_ZLING_DEFAULT_LEVEL 0

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

enum SquashZlingOptIndex {
  SQUASH_ZLING_OPT_LEVEL = 0
};

static SquashOptionInfo squash_zling_options[] = {
  { (char*) "level",
    SQUASH_OPTION_TYPE_RANGE_INT, },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

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

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecFuncs* funcs);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init           (SquashPlugin* plugin);

static void                squash_zling_stream_init     (SquashZlingStream* stream,
                                                         SquashCodec* codec,
                                                         SquashStreamType stream_type,
                                                         SquashOptions* options,
                                                         SquashDestroyNotify destroy_notify);
static SquashZlingStream*  squash_zling_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
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

static SquashZlingStream* squash_zling_stream_new (SquashCodec* codec,
                                                   SquashStreamType stream_type,
                                                   SquashOptions* options) {
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
                          SquashOptions* options,
                          SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, options, destroy_notify);

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
  return (SquashStream*) squash_zling_stream_new (codec, stream_type, options);
}

static SquashStatus
squash_zling_process_stream (SquashStream* stream, SquashOperation operation) {
  SquashStatus res = SQUASH_OK;
  SquashZlingStream* s = (SquashZlingStream*) stream;

  s->operation = operation;

  try {
    if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
      int level = SQUASH_ZLING_DEFAULT_LEVEL;
      if (stream->options != NULL)
        level = squash_codec_get_option_int_index (stream->codec, stream->options, SQUASH_ZLING_OPT_LEVEL);

      res = baidu::zling::Encode(s->stream, s->stream, NULL, level) == 0 ?
        SQUASH_OK : squash_error (SQUASH_FAILED);
    } else {
      res = baidu::zling::Decode(s->stream, s->stream, NULL) == 0 ?
        SQUASH_OK : squash_error (SQUASH_FAILED);
    }
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (...) {
    return squash_error (SQUASH_FAILED);
  }

  return res;
}

static size_t
squash_zling_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return
    uncompressed_length + 288 + (uncompressed_length / 8);
}

extern "C" SquashStatus
squash_plugin_init (SquashPlugin* plugin) {
  const SquashOptionInfoRangeInt level_range = { 0, 4, 0, false };

  squash_zling_options[SQUASH_ZLING_OPT_LEVEL].default_value.int_value = 0;
  squash_zling_options[SQUASH_ZLING_OPT_LEVEL].info.range_int = level_range;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("zling", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_RUN_IN_THREAD;
    funcs->options = squash_zling_options;
    funcs->create_stream = squash_zling_create_stream;
    funcs->process_stream = squash_zling_process_stream;
    funcs->get_max_compressed_size = squash_zling_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
