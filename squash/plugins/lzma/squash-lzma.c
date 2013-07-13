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

#include "squash-lzma.h"

SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecFuncs* funcs);

static SquashLZMAType     squash_lzma_codec_to_type   (SquashCodec* codec);

static void               squash_lzma_options_init    (SquashLZMAOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashLZMAOptions* squash_lzma_options_new     (SquashCodec* codec);
static void               squash_lzma_options_destroy (void* options);
static void               squash_lzma_options_free    (void* options);

static void               squash_lzma_stream_init     (SquashLZMAStream* stream,
																											 SquashCodec* codec,
                                                       SquashLZMAType type,
                                                       SquashStreamType stream_type,
                                                       SquashLZMAOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashLZMAStream*  squash_lzma_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashLZMAOptions* options);
static void               squash_lzma_stream_destroy  (void* stream);
static void               squash_lzma_stream_free     (void* stream);

static SquashLZMAType squash_lzma_codec_to_type (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);
  if (strcmp ("xz", name) == 0) {
    return SQUASH_LZMA_TYPE_XZ;
  } else if (strcmp ("lzma2", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA2;
  } else if (strcmp ("lzma", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA;
  } else if (strcmp ("lzma1", name) == 0) {
    return SQUASH_LZMA_TYPE_LZMA1;
  } else {
    assert (false);
  }
}

static void
squash_lzma_options_init (SquashLZMAOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->type = squash_lzma_codec_to_type (codec);
  options->check = LZMA_CHECK_CRC64;
  options->memlimit = UINT64_MAX;
  lzma_lzma_preset (&(options->options), LZMA_PRESET_DEFAULT);
}

static SquashLZMAOptions*
squash_lzma_options_new (SquashCodec* codec) {
  SquashLZMAOptions* options;

  options = (SquashLZMAOptions*) malloc (sizeof (SquashLZMAOptions));
  squash_lzma_options_init (options, codec, squash_lzma_options_free);

  return options;
}

static void
squash_lzma_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_lzma_options_free (void* options) {
  squash_lzma_options_destroy ((SquashLZMAOptions*) options);
  free (options);
}

static SquashOptions*
squash_lzma_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_lzma_options_new (codec);
}

static SquashStatus
squash_lzma_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashStatus res = SQUASH_BAD_PARAM;
  SquashLZMAOptions* opts = (SquashLZMAOptions*) options;
  char* endptr = NULL;

  assert (options != NULL);

  if (strcasecmp (key, "level") == 0) {
    const uint32_t level = (uint32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 1 && level <= 9 ) {
      lzma_lzma_preset (&(opts->options), level);
    } else {
      return SQUASH_BAD_VALUE;
    }
    res = SQUASH_OK;
  } else if (strcasecmp (key, "dict-size") == 0) {
    const uint32_t dict_size = (uint32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && dict_size >= 4096 && dict_size <= 1610612736 ) {
      opts->options.dict_size = dict_size;
    } else {
      return SQUASH_BAD_VALUE;
    }
    res = SQUASH_OK;
  } else if (strcasecmp (key, "lc") == 0) {
    const uint32_t lc = (uint32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && lc >= 0 && lc <= 4 ) {
      opts->options.lc = lc;
    } else {
      return SQUASH_BAD_VALUE;
    }
    res = SQUASH_OK;
  } else if (strcasecmp (key, "lp") == 0) {
    const uint32_t lp = (uint32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && lp >= 0 && lp <= 4 ) {
      opts->options.lp = lp;
    } else {
      return SQUASH_BAD_VALUE;
    }
    res = SQUASH_OK;
  } else if (strcasecmp (key, "pb") == 0) {
    const uint32_t pb = (uint32_t) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && pb >= 0 && pb <= 4 ) {
      opts->options.pb = pb;
    } else {
      return SQUASH_BAD_VALUE;
    }
    res = SQUASH_OK;
  } else if (opts->type == SQUASH_LZMA_TYPE_XZ) {
    if (strcasecmp (key, "check") == 0) {
      const lzma_check check = (lzma_check) strtol (value, &endptr, 0);
      if ( *endptr == '\0' ) {
        if (lzma_check_is_supported (check)) {
          opts->check = check;
        } else {
          return SQUASH_BAD_VALUE;
        }
      } else if (strcasecmp (value, "none") == 0) {
        opts->check = LZMA_CHECK_NONE;
      } else if (strcasecmp (value, "crc32") == 0) {
        opts->check = LZMA_CHECK_CRC32;
      } else if (strcasecmp (value, "crc64") == 0) {
        opts->check = LZMA_CHECK_CRC64;
      } else if (strcasecmp (value, "sha256") == 0) {
        opts->check = LZMA_CHECK_SHA256;
      } else {
        return SQUASH_BAD_VALUE;
      }
      res = SQUASH_OK;
    }
  }

  return res;
}

static void
squash_lzma_stream_init (SquashLZMAStream* stream,
                      SquashCodec* codec,
                      SquashLZMAType type,
                      SquashStreamType stream_type,
                      SquashLZMAOptions* options,
                      SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  lzma_stream s = LZMA_STREAM_INIT;
  stream->stream = s;
  stream->type = type;
}

static void
squash_lzma_stream_destroy (void* stream) {
  lzma_end (&((SquashLZMAStream*) stream)->stream);
  squash_stream_destroy (stream);
}

static void
squash_lzma_stream_free (void* stream) {
  squash_lzma_stream_destroy (stream);
  free (stream);
}

static SquashLZMAStream*
squash_lzma_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashLZMAOptions* options) {
  lzma_ret lzma_e;
  SquashLZMAStream* stream;
  SquashLZMAType lzma_type;
  lzma_options_lzma lzma_options;
  lzma_filter filters[2];

  assert (codec != NULL);

  if (options != NULL) {
    lzma_type = options->type;
    filters[0].options = &(options->options);
  } else {
    lzma_type = squash_lzma_codec_to_type (codec);
    lzma_lzma_preset (&lzma_options, LZMA_PRESET_DEFAULT);
    filters[0].options = &(lzma_options);
  }

  switch (lzma_type) {
    case SQUASH_LZMA_TYPE_XZ:
    case SQUASH_LZMA_TYPE_LZMA2:
      filters[0].id = LZMA_FILTER_LZMA2;
      break;
    case SQUASH_LZMA_TYPE_LZMA:
    case SQUASH_LZMA_TYPE_LZMA1:
      filters[0].id = LZMA_FILTER_LZMA1;
      break;
  }

  filters[1].id = LZMA_VLI_UNKNOWN;
  filters[1].options = NULL;

  stream = (SquashLZMAStream*) malloc (sizeof (SquashLZMAStream));
  squash_lzma_stream_init (stream, codec, lzma_type, stream_type, options, squash_lzma_stream_free);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    if (lzma_type == SQUASH_LZMA_TYPE_XZ) {
      lzma_e = lzma_stream_encoder (&(stream->stream), filters, (options != NULL) ? options->check : LZMA_CHECK_CRC64);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA) {
      lzma_e = lzma_alone_encoder (&(stream->stream), filters[0].options);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA1 ||
               lzma_type == SQUASH_LZMA_TYPE_LZMA2) {
			lzma_e = lzma_raw_encoder(&(stream->stream), filters);
    } else {
      assert (false);
    }
  } else if (stream_type == SQUASH_STREAM_DECOMPRESS) {
    const uint64_t memlimit = (options != NULL) ? options->memlimit : UINT64_MAX;

    if (lzma_type == SQUASH_LZMA_TYPE_XZ) {
			lzma_e = lzma_stream_decoder(&(stream->stream), memlimit, 0);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA) {
			lzma_e = lzma_alone_decoder(&(stream->stream), memlimit);
    } else if (lzma_type == SQUASH_LZMA_TYPE_LZMA1 ||
               lzma_type == SQUASH_LZMA_TYPE_LZMA2) {
			lzma_e = lzma_raw_decoder(&(stream->stream), filters);
    } else {
      assert (false);
    }
  } else {
    assert (false);
  }

  if (lzma_e != LZMA_OK) {
    stream = squash_object_unref (stream), NULL;
  }

  return stream;
}

static SquashStream*
squash_lzma_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_lzma_stream_new (codec, stream_type, (SquashLZMAOptions*) options);
}

#define SQUASH_LZMA_STREAM_COPY_TO_LZMA_STREAM(stream,lzma_stream) \
  lzma_stream->next_in =  stream->next_in;                      \
  lzma_stream->avail_in = stream->avail_in;                     \
  lzma_stream->next_out = stream->next_out;                     \
  lzma_stream->avail_out = stream->avail_out

#define SQUASH_LZMA_STREAM_COPY_FROM_LZMA_STREAM(stream,lzma_stream) \
  stream->next_in = lzma_stream->next_in;                       \
  stream->avail_in = lzma_stream->avail_in;                       \
  stream->next_out = lzma_stream->next_out;                       \
  stream->avail_out = lzma_stream->avail_out

static SquashStatus
squash_lzma_process_stream (SquashStream* stream) {
  lzma_stream* lzma_stream;
  lzma_ret lzma_e;

  assert (stream != NULL);
  lzma_stream = &(((SquashLZMAStream*) stream)->stream);

  SQUASH_LZMA_STREAM_COPY_TO_LZMA_STREAM(stream, lzma_stream);
  lzma_e = lzma_code (lzma_stream, LZMA_RUN);
  SQUASH_LZMA_STREAM_COPY_FROM_LZMA_STREAM(stream, lzma_stream);

  if (lzma_e == LZMA_OK) {
    return SQUASH_OK;
  } else if (lzma_e == LZMA_STREAM_END) {
    return SQUASH_END_OF_STREAM;
  } else {
    return SQUASH_FAILED;
  }
}

/* static SquashStatus */
/* squash_lzma_flush_stream (SquashStream* stream) { */
/*   return SQUASH_OK; */
/* } */

static SquashStatus
squash_lzma_finish_stream (SquashStream* stream) {
  lzma_stream* lzma_stream;
  lzma_ret lzma_e;

  assert (stream != NULL);
  lzma_stream = &(((SquashLZMAStream*) stream)->stream);

  SQUASH_LZMA_STREAM_COPY_TO_LZMA_STREAM(stream, lzma_stream);
  lzma_e = lzma_code (lzma_stream, LZMA_FINISH);
  SQUASH_LZMA_STREAM_COPY_FROM_LZMA_STREAM(stream, lzma_stream);

  if (lzma_e == LZMA_OK) {
    return SQUASH_PROCESSING;
  } else if (lzma_e == LZMA_STREAM_END) {
    return SQUASH_OK;
  } else {
    return SQUASH_FAILED;
  }
}

static size_t
squash_lzma_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return lzma_stream_buffer_bound (uncompressed_length);
}

static SquashCodecFuncs squash_lzma_codec_funcs = {
  squash_lzma_create_options,
  squash_lzma_parse_option,
  squash_lzma_create_stream,
  squash_lzma_process_stream,
  NULL, /* flush_stream */
  squash_lzma_finish_stream,
  NULL, /* get_uncompressed_size */
  squash_lzma_get_max_compressed_size,
  NULL, /* decompress_buffer */
  NULL /* compress_buffer */
};

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  *funcs = squash_lzma_codec_funcs;

  return SQUASH_OK;
}
