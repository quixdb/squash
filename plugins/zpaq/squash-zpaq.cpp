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

#if !defined(_MSC_VER)
#include <strings.h>
#else
#define snprintf _snprintf
#endif

#include <squash/squash.h>

#include "libzpaq.h"

#define SQUASH_ZPAQ_DEFAULT_LEVEL 1

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
  void* user_data_;
  SquashReadFunc reader_;
  SquashWriteFunc writer_;

  int get ();
  void put (int c);

  int read (char* buf, int n);
  void write (const char* buf, int n);

  SquashZpaqIO(void* user_data, SquashReadFunc reader, SquashWriteFunc writer) :
    user_data_ (user_data),
    reader_ (reader),
    writer_ (writer) { }
};

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecImpl* impl);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_plugin   (SquashPlugin* plugin);

#if defined(__GNUC__)
__attribute__((__noreturn__))
#endif
void
libzpaq::error (const char* msg) {
  if (strcmp (msg, "Out of memory") == 0)
    throw squash_error (SQUASH_MEMORY);
  else
    throw squash_error (SQUASH_FAILED);
}

int SquashZpaqIO::get () {
  char v;
  return (this->read(&v, 1)) ? (int) v : -1;
}

int SquashZpaqIO::read (char* buf, int size) {
  size_t l = (size_t) size;
  this->reader_ (&l, (uint8_t*) buf, this->user_data_);
  return l;
}

void SquashZpaqIO::put (int c) {
  uint8_t v = (uint8_t) c;
  this->write ((const char*) &v, 1);
}

void SquashZpaqIO::write (const char* buf, int n) {
  size_t s = (size_t) n;
  SquashStatus res = this->writer_ (&s, (const uint8_t*) buf, this->user_data_);
  if (res != SQUASH_OK)
    throw res;
}

static SquashStatus
squash_zpaq_splice (SquashCodec* codec,
                    SquashOptions* options,
                    SquashStreamType stream_type,
                    SquashReadFunc read_cb,
                    SquashWriteFunc write_cb,
                    void* user_data) {
  try {
    SquashZpaqIO stream(user_data, read_cb, write_cb);
    if (stream_type == SQUASH_STREAM_COMPRESS) {
      char level_s[3] = { 0, };
      snprintf (level_s, sizeof(level_s), "%d", squash_codec_get_option_int_index (codec, options, SQUASH_ZPAQ_OPT_LEVEL));

      compress (&stream, &stream, level_s);
    } else {
      decompress (&stream, &stream);
    }
  } catch (const std::bad_alloc& e) {
    return squash_error (SQUASH_MEMORY);
  } catch (const SquashStatus e) {
    return e;
  } catch (...) {
    return SQUASH_FAILED;
  }

  return SQUASH_OK;
}

static size_t
squash_zpaq_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return
    uncompressed_size +
    ((uncompressed_size / 100) * 1) + ((uncompressed_size % 100) > 0 ? 1 : 0) +
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
    impl->options = squash_zpaq_options;
    impl->splice = squash_zpaq_splice;
    impl->get_max_compressed_size = squash_zpaq_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
