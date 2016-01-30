/* Copyright (c) 2013-2016 The Squash Authors
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

#include "crush.h"

struct SquashCrushData {
  void* user_data;
  SquashReadFunc reader;
  SquashWriteFunc writer;
  SquashStatus last_res;
  SquashContext* context;
};

enum SquashCrushOptIndex {
  SQUASH_CRUSH_OPT_LEVEL = 0
};

static SquashOptionInfo squash_crush_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 2 },
    .default_value.int_value = 1 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

SQUASH_PLUGIN_EXPORT
SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecImpl* impl);

static void*
squash_crush_malloc (size_t size, void* user_data) {
  return squash_calloc (size, 1);
}

static void
squash_crush_free (void* ptr, void* user_data) {
  return squash_free (ptr);
}

static size_t
squash_crush_reader (void* buffer, size_t size, void* user_data) {
  struct SquashCrushData* data = (struct SquashCrushData*) user_data;

  /* CRUSH doesn't check the return value from write calls, so it
     doesn't detect that there is a problem.  We need to be able to
     report the error instead, though, so just act like we have read
     all the input we want and don't clobber the last status code. */

  if (data->last_res > 0) {
    data->last_res = data->reader (&size, buffer, data->user_data);
    return size;
  } else {
    return 0;
  }
}

static size_t
squash_crush_writer (const void* buffer, size_t size, void* user_data) {
  struct SquashCrushData* data = (struct SquashCrushData*) user_data;

  if (data->last_res > 0) {
    data->last_res = data->writer (&size, buffer, data->user_data);
    return size;
  } else {
    return 0;
  }
}

static SquashStatus
squash_crush_splice (SquashCodec* codec,
                     SquashOptions* options,
                     SquashStreamType stream_type,
                     SquashReadFunc read_cb,
                     SquashWriteFunc write_cb,
                     void* user_data) {
  struct SquashCrushData data = {
    user_data,
    read_cb,
    write_cb,
    SQUASH_OK,
    squash_codec_get_context (codec)
  };
  CrushContext ctx;
  int res;

  res = crush_init_full (&ctx, squash_crush_reader, squash_crush_writer, squash_crush_malloc, squash_crush_free, &data, NULL);
  if (res != 0)
    return squash_error (SQUASH_FAILED);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    res = crush_compress (&ctx, squash_options_get_int_at (options, codec, SQUASH_CRUSH_OPT_LEVEL));
  } else {
    res = crush_decompress (&ctx);
  }

  crush_destroy (&ctx);

  if (data.last_res < 0)
    return data.last_res;
  else if (SQUASH_UNLIKELY(res != 0))
    return squash_error (SQUASH_FAILED);
  else
    return SQUASH_OK;
}

static size_t
squash_crush_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return
    uncompressed_size + 4 + (uncompressed_size / 7) + ((uncompressed_size % 7) == 0 ? 0 : 1);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("crush", name) == 0)) {
    impl->options = squash_crush_options;
    impl->splice = squash_crush_splice;
    impl->get_max_compressed_size = squash_crush_get_max_compressed_size;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
