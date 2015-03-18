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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <squash/squash.h>
#include <lz4.h>
#include <lz4frame.h>

#define SQUASH_LZ4F_DICT_SIZE ((size_t) 65536)

typedef struct SquashLZ4FOptions_s {
  SquashOptions base_object;

  LZ4F_preferences_t prefs;
} SquashLZ4FOptions;

enum SquashLZ4FState {
  SQUASH_LZ4F_STATE_INIT,
  SQUASH_LZ4F_STATE_ACTIVE,
  SQUASH_LZ4F_STATE_FINISHED,
  SQUASH_LZ4F_BUFFERING,
};

typedef struct SquashLZ4FStream_s {
  SquashStream base_object;

  union {
    struct {
      LZ4F_compressionContext_t ctx;

      enum SquashLZ4FState state;

      uint8_t* output_buffer;
      size_t output_buffer_pos;
      size_t output_buffer_length;

      size_t input_buffer_length;
    } comp;

    struct {
      LZ4F_decompressionContext_t ctx;
    } decomp;
  } data;
} SquashLZ4FStream;

SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecFuncs* funcs);

static void               squash_lz4f_options_init    (SquashLZ4FOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashLZ4FOptions* squash_lz4f_options_new     (SquashCodec* codec);
static void               squash_lz4f_options_destroy (void* options);
static void               squash_lz4f_options_free    (void* options);

static void               squash_lz4f_stream_init     (SquashLZ4FStream* stream,
                                                       SquashCodec* codec,
                                                       SquashStreamType stream_type,
                                                       SquashLZ4FOptions* options,
                                                       SquashDestroyNotify destroy_notify);
static SquashLZ4FStream*  squash_lz4f_stream_new      (SquashCodec* codec, SquashStreamType stream_type, SquashLZ4FOptions* options);
static void               squash_lz4f_stream_destroy  (void* stream);
static void               squash_lz4f_stream_free     (void* stream);

SquashStatus              squash_plugin_init_codec   (SquashCodec* codec,
                                                      SquashCodecFuncs* funcs);
static const LZ4F_preferences_t squash_lz4f_default_preferences = {
  { max64KB, blockLinked, noContentChecksum, },
  0, 0,
};

static void
squash_lz4f_options_init (SquashLZ4FOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->prefs = squash_lz4f_default_preferences;
}

static SquashLZ4FOptions*
squash_lz4f_options_new (SquashCodec* codec) {
  SquashLZ4FOptions* options;

  options = (SquashLZ4FOptions*) malloc (sizeof (SquashLZ4FOptions));
  squash_lz4f_options_init (options, codec, squash_lz4f_options_free);

  return options;
}

static void
squash_lz4f_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_lz4f_options_free (void* options) {
  squash_lz4f_options_destroy ((SquashLZ4FOptions*) options);
  free (options);
}

static SquashOptions*
squash_lz4f_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_lz4f_options_new (codec);
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
squash_lz4f_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashLZ4FOptions* opts = (SquashLZ4FOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = (int) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 0 && level <= 16) {
      opts->prefs.compressionLevel = (unsigned int) level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "block-size") == 0) {
    const int bs = (int) strtol (value, &endptr, 0);
    if (*endptr != '\0')
      return SQUASH_BAD_VALUE;

    switch (bs) {
      case max64KB:
      case max256KB:
      case max1MB:
      case max4MB:
        opts->prefs.frameInfo.blockSizeID = (blockSizeID_t) bs;
        break;
      default:
        return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "checksum")) {
    bool res;
    if (string_to_bool(value, &res)) {
      opts->prefs.frameInfo.contentChecksumFlag = res;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static SquashLZ4FStream*
squash_lz4f_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashLZ4FOptions* options) {
  SquashLZ4FStream* stream;

  assert (codec != NULL);

  stream = (SquashLZ4FStream*) malloc (sizeof (SquashLZ4FStream));
  squash_lz4f_stream_init (stream, codec, stream_type, options, squash_lz4f_stream_free);

  return stream;
}

#define SQUASH_LZ4F_STREAM_IS_HC(s) \
  (((((SquashStream*) s)->options) != NULL) && (((SquashLZ4FOptions*) (((SquashStream*) s)->options))->hc))

static void
squash_lz4f_stream_init (SquashLZ4FStream* stream,
                         SquashCodec* codec,
                         SquashStreamType stream_type,
                         SquashLZ4FOptions* options,
                         SquashDestroyNotify destroy_notify) {
  LZ4F_errorCode_t ec;
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    ec = LZ4F_createCompressionContext(&(stream->data.comp.ctx), LZ4F_VERSION);

    stream->data.comp.state = SQUASH_LZ4F_STATE_INIT;

    stream->data.comp.output_buffer = NULL;
    stream->data.comp.output_buffer_pos = 0;
    stream->data.comp.output_buffer_length = 0;

    stream->data.comp.input_buffer_length = 0;
  } else {
    ec = LZ4F_createDecompressionContext(&(stream->data.decomp.ctx), LZ4F_VERSION);
  }

  assert (!LZ4F_isError (ec));
}

static void
squash_lz4f_stream_destroy (void* stream) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;
  LZ4F_errorCode_t ec;

  if (((SquashStream*) stream)->stream_type == SQUASH_STREAM_COMPRESS) {
    ec = LZ4F_freeCompressionContext(s->data.comp.ctx);

    if (s->data.comp.output_buffer != NULL)
      free (s->data.comp.output_buffer);
  } else {
    ec = LZ4F_freeDecompressionContext(s->data.decomp.ctx);
  }

  assert (!LZ4F_isError (ec));

  squash_stream_destroy (stream);
}

static void
squash_lz4f_stream_free (void* stream) {
  squash_lz4f_stream_destroy (stream);
  free (stream);
}

static SquashStream*
squash_lz4f_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_lz4f_stream_new (codec, stream_type, (SquashLZ4FOptions*) options);
}

static size_t
squash_lz4f_block_size_id_to_size (blockSizeID_t blkid) {
  switch (blkid) {
    case max64KB:
      return  64 * 1024;
    case max256KB:
      return 256 * 1024;
    case max1MB:
      return   1 * 1024 * 1024;
    case max4MB:
      return   4 * 1024 * 1024;
    default:
      assert (0);
      break;
  }
}

static size_t
squash_lz4f_get_input_buffer_size (SquashStream* stream) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;

  if (stream->options != NULL)
    return squash_lz4f_block_size_id_to_size (((SquashLZ4FOptions*) stream->options)->prefs.frameInfo.blockSizeID);

  return 64 * 1024;
}

static const LZ4F_preferences_t*
squash_lz4f_stream_get_prefs (SquashStream* stream) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;
  SquashLZ4FOptions* opts = (SquashLZ4FOptions*) stream->options;

  return (opts == NULL) ? &squash_lz4f_default_preferences : &(opts->prefs);
}

static size_t
squash_lz4f_stream_get_output_buffer_size (SquashStream* stream) {
  /* I'm pretty sure there is a very overly ambitious check in
     LZ4F_compressFrame when srcSize == blockSize, but not much we can
     do about it here.  It just means LZ4F will do some extra
     memcpy()ing (for output buffers up to a bit over double the block
     size). */
  return LZ4F_compressFrameBound(squash_lz4f_get_input_buffer_size (stream) * 2, squash_lz4f_stream_get_prefs (stream));
}

static uint8_t*
squash_lz4f_stream_get_output_buffer (SquashStream* stream) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;

  if (s->data.comp.output_buffer == NULL)
    s->data.comp.output_buffer = malloc (squash_lz4f_stream_get_output_buffer_size (stream));

  return s->data.comp.output_buffer;
}

static SquashStatus
squash_lz4f_compress_stream (SquashStream* stream, SquashOperation operation) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;
  bool progress = false;

  if (s->data.comp.output_buffer_length != 0) {
    const size_t buffer_remaining = s->data.comp.output_buffer_length - s->data.comp.output_buffer_pos;
    const size_t cp_size = (buffer_remaining < stream->avail_out) ? buffer_remaining : stream->avail_out;

    memcpy (stream->next_out, s->data.comp.output_buffer + s->data.comp.output_buffer_pos, cp_size);
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;
    s->data.comp.output_buffer_pos += cp_size;

    if (cp_size == buffer_remaining) {
      s->data.comp.output_buffer_length = 0;
      s->data.comp.output_buffer_pos = 0;

      progress = true;
    } else {
      return SQUASH_PROCESSING;
    }
  }

  while ((stream->avail_in != 0 || operation != SQUASH_OPERATION_PROCESS) && stream->avail_out != 0) {
    if (s->data.comp.state == SQUASH_LZ4F_STATE_INIT) {
      s->data.comp.state = SQUASH_LZ4F_STATE_ACTIVE;
      if (stream->avail_out < 19) {
        s->data.comp.output_buffer_length =
          LZ4F_compressBegin (s->data.comp.ctx,
                              squash_lz4f_stream_get_output_buffer (stream),
                              squash_lz4f_stream_get_output_buffer_size (stream),
                              squash_lz4f_stream_get_prefs (stream));
        break;
      } else {
        size_t written = LZ4F_compressBegin (s->data.comp.ctx, stream->next_out, stream->avail_out, squash_lz4f_stream_get_prefs (stream));
        stream->next_out += written;
        stream->avail_out -= written;
        progress = true;
      }
    } else {
      const size_t input_buffer_size = squash_lz4f_get_input_buffer_size (stream);
      const size_t total_input = stream->avail_in + s->data.comp.input_buffer_length;
      const size_t output_buffer_max_length = squash_lz4f_stream_get_output_buffer_size (stream);

      if (progress && (total_input < input_buffer_size || stream->avail_out < output_buffer_max_length))
        break;

      uint8_t* obuf;
      size_t olen;

      const size_t input_length = (total_input > input_buffer_size) ? (input_buffer_size - s->data.comp.input_buffer_length) : stream->avail_in;
      if (input_length > 0) {
        obuf = (output_buffer_max_length > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream) : stream->next_out;
        olen = LZ4F_compressUpdate (s->data.comp.ctx, obuf, output_buffer_max_length, stream->next_in, input_length, NULL);

        if (!LZ4F_isError (olen)) {
          if (input_length + s->data.comp.input_buffer_length == input_buffer_size) {
            s->data.comp.input_buffer_length = 0;
          } else {
            s->data.comp.input_buffer_length += input_length;
            assert (olen == 0);
          }

          stream->next_in += input_length;
          stream->avail_in -= input_length;
        } else {
          assert (0);
        }
      } else if (operation == SQUASH_OPERATION_FLUSH) {
        assert (stream->avail_in == 0);
        olen = squash_lz4f_stream_get_output_buffer_size (stream);
        obuf = (olen > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream) : stream->next_out;
        olen = LZ4F_flush (s->data.comp.ctx, obuf, olen, NULL);

        s->data.comp.input_buffer_length = 0;
      } else if (operation == SQUASH_OPERATION_FINISH) {
        assert (stream->avail_in == 0);
        olen = squash_lz4f_stream_get_output_buffer_size (stream);
        obuf = (olen > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream) : stream->next_out;
        olen = LZ4F_compressEnd (s->data.comp.ctx, obuf, olen, NULL);

        s->data.comp.input_buffer_length = 0;
      } else if (progress) {
        break;
      } else {
        assert (0);
      }

      if (LZ4F_isError (olen)) {
        assert (0);
        return SQUASH_FAILED;
      } else {
        if (olen != 0) {
          if (obuf == s->data.comp.output_buffer) {
            s->data.comp.output_buffer_length = olen;
          } else {
            stream->next_out += olen;
            stream->avail_out -= olen;
          }
        }
      }

      if (operation != SQUASH_OPERATION_PROCESS)
        break;
    }
  }

  if (s->data.comp.output_buffer_length != 0) {
    const size_t buffer_remaining = s->data.comp.output_buffer_length - s->data.comp.output_buffer_pos;
    const size_t cp_size = (buffer_remaining < stream->avail_out) ? buffer_remaining : stream->avail_out;

    memcpy (stream->next_out, s->data.comp.output_buffer + s->data.comp.output_buffer_pos, cp_size);
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;
    s->data.comp.output_buffer_pos += cp_size;

    if (cp_size == buffer_remaining) {
      s->data.comp.output_buffer_length = 0;
      s->data.comp.output_buffer_pos = 0;

      progress = true;
    } else {
      return SQUASH_PROCESSING;
    }
  }

  return (stream->avail_in == 0 && s->data.comp.output_buffer_length == 0) ? SQUASH_OK : SQUASH_PROCESSING;
}

static SquashStatus
squash_lz4f_decompress_stream (SquashStream* stream, SquashOperation operation) {
  SquashLZ4FStream* s = (SquashLZ4FStream*) stream;

  while (stream->avail_in != 0 && stream->avail_out != 0) {
    size_t dst_len = stream->avail_out;
    size_t src_len = stream->avail_in;
    size_t bytes_read = LZ4F_decompress (s->data.decomp.ctx, stream->next_out, &dst_len, stream->next_in, &src_len, NULL);

    if (src_len != 0) {
      stream->next_in += src_len;
      stream->avail_in -= src_len;
    }

    if (dst_len != 0) {
      stream->next_out += dst_len;
      stream->avail_out -= dst_len;
    }
  }

  return (stream->avail_in == 0) ? SQUASH_OK : SQUASH_PROCESSING;
}

static SquashStatus
squash_lz4f_process_stream (SquashStream* stream, SquashOperation operation) {
  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      return squash_lz4f_compress_stream (stream, operation);
    case SQUASH_STREAM_DECOMPRESS:
      return squash_lz4f_decompress_stream (stream, operation);
    default:
      assert (false);
  }
}

static size_t
squash_lz4f_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  static const LZ4F_preferences_t prefs = {
    { max64KB, blockLinked, contentChecksumEnabled, },
    0, 0,
  };

  const size_t block_size = squash_lz4f_block_size_id_to_size (prefs.frameInfo.blockSizeID);
  const size_t full_blocks = uncompressed_length / block_size;
  const size_t last_block = ((uncompressed_length % block_size) == 0) ? block_size : (uncompressed_length % block_size);
  const size_t block_overhead = 8;

  const size_t res =
    (full_blocks * (block_overhead + block_size)) +
    (last_block == 0 ? 0 : (block_overhead + last_block))
    + 7;

  return res;
}

SquashStatus
squash_plugin_init_lz4f (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("lz4f", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    funcs->create_options = squash_lz4f_create_options;
    funcs->parse_option = squash_lz4f_parse_option;
    funcs->get_max_compressed_size = squash_lz4f_get_max_compressed_size;
    funcs->create_stream = squash_lz4f_create_stream;
    funcs->process_stream = squash_lz4f_process_stream;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
