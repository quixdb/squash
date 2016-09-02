/* Copyright (c) 2015-2016 The Squash Authors
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
#include <errno.h>

#include <squash/squash.h>
#include <lz4.h>
#include <lz4frame_static.h>

SquashStatus squash_plugin_init_lz4f (SquashCodec* codec, SquashCodecImpl* impl);

#define SQUASH_LZ4F_DICT_SIZE ((size_t) 65536)

enum SquashLZ4FOptIndex {
  SQUASH_LZ4F_OPT_LEVEL = 0,
  SQUASH_LZ4F_OPT_BLOCK_SIZE,
  SQUASH_LZ4F_OPT_CHECKSUM,
};

static SquashOptionInfo squash_lz4f_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 0,
      .max = 16 },
    .default_value.int_value = 0 },
  { "block-size",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 4,
      .max = 7 },
    .default_value.int_value = 4 },
  { "checksum",
    SQUASH_OPTION_TYPE_BOOL,
    .default_value.bool_value = false },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

enum SquashLZ4FState {
  SQUASH_LZ4F_STATE_INIT,
  SQUASH_LZ4F_STATE_ACTIVE,
  SQUASH_LZ4F_STATE_FINISHED,
  SQUASH_LZ4F_BUFFERING,
};

typedef union {
  struct {
    LZ4F_compressionContext_t ctx;
    LZ4F_preferences_t prefs;

    enum SquashLZ4FState state;

    uint8_t* output_buffer;
    size_t output_buffer_pos;
    size_t output_buffer_size;

    size_t input_buffer_size;
  } comp;

  struct {
    LZ4F_decompressionContext_t ctx;
  } decomp;
} SquashLZ4FStream;

SQUASH_PLUGIN_EXPORT
SquashStatus              squash_plugin_init_codec    (SquashCodec* codec, SquashCodecImpl* impl);

static SquashStatus
squash_lz4f_get_status (size_t res) {
  if (!LZ4F_isError (res))
    return SQUASH_OK;

  switch ((LZ4F_errorCodes) (-(int)(res))) {
    case LZ4F_OK_NoError:
      return SQUASH_OK;
    case LZ4F_ERROR_GENERIC:
      return squash_error (SQUASH_FAILED);
    case LZ4F_ERROR_maxBlockSize_invalid:
    case LZ4F_ERROR_blockMode_invalid:
    case LZ4F_ERROR_contentChecksumFlag_invalid:
    case LZ4F_ERROR_headerVersion_wrong:
    case LZ4F_ERROR_blockChecksum_unsupported:
    case LZ4F_ERROR_reservedFlag_set:
    case LZ4F_ERROR_frameHeader_incomplete:
    case LZ4F_ERROR_frameType_unknown:
    case LZ4F_ERROR_frameSize_wrong:
    case LZ4F_ERROR_headerChecksum_invalid:
    case LZ4F_ERROR_contentChecksum_invalid:
      return squash_error (SQUASH_INVALID_BUFFER);
    case LZ4F_ERROR_compressionLevel_invalid:
      return squash_error (SQUASH_BAD_VALUE);
    case LZ4F_ERROR_allocation_failed:
      return squash_error (SQUASH_MEMORY);
    case LZ4F_ERROR_srcSize_tooLarge:
    case LZ4F_ERROR_dstMaxSize_tooSmall:
      return squash_error (SQUASH_BUFFER_FULL);
    case LZ4F_ERROR_decompressionFailed:
    case LZ4F_ERROR_srcPtr_wrong:
    case LZ4F_ERROR_maxCode:
      return squash_error (SQUASH_FAILED);
    default:
      HEDLEY_UNREACHABLE ();
  }
}

static bool
squash_lz4f_init_stream (SquashStream* stream,
                         SquashStreamType stream_type,
                         SquashOptions* options,
                         void* priv) {
  SquashLZ4FStream* s = priv;
  LZ4F_errorCode_t ec;
  SquashCodec* codec = stream->codec;

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    ec = LZ4F_createCompressionContext(&(s->comp.ctx), LZ4F_VERSION);

    s->comp.state = SQUASH_LZ4F_STATE_INIT;

    s->comp.output_buffer = NULL;
    s->comp.output_buffer_pos = 0;
    s->comp.output_buffer_size = 0;

    s->comp.input_buffer_size = 0;

    s->comp.prefs = (LZ4F_preferences_t) {
      {
        (LZ4F_blockSizeID_t) squash_options_get_int_at (options, codec, SQUASH_LZ4F_OPT_BLOCK_SIZE),
        blockLinked,
        squash_options_get_bool_at (options, codec, SQUASH_LZ4F_OPT_CHECKSUM) ?
          contentChecksumEnabled :
          noContentChecksum,
      },
      squash_options_get_int_at (options, codec, SQUASH_LZ4F_OPT_LEVEL)
    };
  } else {
    ec = LZ4F_createDecompressionContext(&(s->decomp.ctx), LZ4F_VERSION);
  }

  if (HEDLEY_UNLIKELY(LZ4F_isError (ec)))
    return (squash_error (SQUASH_FAILED), false);

  return true;
}

#define SQUASH_LZ4F_STREAM_IS_HC(s) \
  (((((SquashStream*) s)->options) != NULL) && (((SquashOptions*) (((SquashStream*) s)->options))->hc))

static void
squash_lz4f_destroy_stream (SquashStream* stream, void* priv) {
  SquashLZ4FStream* s = priv;

  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      LZ4F_freeCompressionContext(s->comp.ctx);

      if (s->comp.output_buffer != NULL)
        squash_free (s->comp.output_buffer);
      break;
    case SQUASH_STREAM_DECOMPRESS:
      LZ4F_freeDecompressionContext(s->decomp.ctx);
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
}

static size_t
squash_lz4f_block_size_id_to_size (LZ4F_blockSizeID_t blkid) {
  switch (blkid) {
    case LZ4F_max64KB:
      return  64 * 1024;
    case LZ4F_max256KB:
      return 256 * 1024;
    case LZ4F_max1MB:
      return   1 * 1024 * 1024;
    case LZ4F_max4MB:
      return   4 * 1024 * 1024;
    case LZ4F_default:
    default:
      HEDLEY_UNREACHABLE();
      break;
  }
}

static size_t
squash_lz4f_get_input_buffer_size (SquashStream* stream) {
  return squash_lz4f_block_size_id_to_size ((LZ4F_blockSizeID_t) squash_options_get_int_at (stream->options, stream->codec, SQUASH_LZ4F_OPT_BLOCK_SIZE));
}

static size_t
squash_lz4f_stream_get_output_buffer_size (SquashStream* stream, SquashLZ4FStream* s) {
  /* I'm pretty sure there is a very overly ambitious check in
     LZ4F_compressFrame when srcSize == blockSize, but not much we can
     do about it here.  It just means LZ4F will do some extra
     memcpy()ing (for output buffers up to a bit over double the block
     size). */
  return LZ4F_compressFrameBound(squash_lz4f_get_input_buffer_size (stream) * 2,
                                 &(s->comp.prefs));
}

static uint8_t*
squash_lz4f_stream_get_output_buffer (SquashStream* stream, SquashLZ4FStream* s) {
  if (s->comp.output_buffer == NULL)
    s->comp.output_buffer = squash_malloc (squash_lz4f_stream_get_output_buffer_size (stream, s));

  return s->comp.output_buffer;
}

static SquashStatus
squash_lz4f_compress_stream (SquashStream* stream, SquashOperation operation, SquashLZ4FStream* s) {
  bool progress = false;

  if (s->comp.output_buffer_size != 0) {
    const size_t buffer_remaining = s->comp.output_buffer_size - s->comp.output_buffer_pos;
    const size_t cp_size = (buffer_remaining < stream->avail_out) ? buffer_remaining : stream->avail_out;

    memcpy (stream->next_out, s->comp.output_buffer + s->comp.output_buffer_pos, cp_size);
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;
    s->comp.output_buffer_pos += cp_size;

    if (cp_size == buffer_remaining) {
      s->comp.output_buffer_size = 0;
      s->comp.output_buffer_pos = 0;

      progress = true;
    } else {
      return SQUASH_PROCESSING;
    }
  }

  while ((stream->avail_in != 0 || operation != SQUASH_OPERATION_PROCESS) && stream->avail_out != 0) {
    if (s->comp.state == SQUASH_LZ4F_STATE_INIT) {
      s->comp.state = SQUASH_LZ4F_STATE_ACTIVE;
      if (stream->avail_out < 19) {
        s->comp.output_buffer_size =
          LZ4F_compressBegin (s->comp.ctx,
                              squash_lz4f_stream_get_output_buffer (stream, s),
                              squash_lz4f_stream_get_output_buffer_size (stream, s),
                              &(s->comp.prefs));
        break;
      } else {
        size_t written = LZ4F_compressBegin (s->comp.ctx, stream->next_out, stream->avail_out, &(s->comp.prefs));
        stream->next_out += written;
        stream->avail_out -= written;
        progress = true;
      }
    } else {
      const size_t input_buffer_size = squash_lz4f_get_input_buffer_size (stream);
      const size_t total_input = stream->avail_in + s->comp.input_buffer_size;
      const size_t output_buffer_max_size = squash_lz4f_stream_get_output_buffer_size (stream, s);

      if (progress && (total_input < input_buffer_size || stream->avail_out < output_buffer_max_size))
        break;

      uint8_t* obuf;
      size_t olen;

      const size_t input_size = (total_input > input_buffer_size) ? (input_buffer_size - s->comp.input_buffer_size) : stream->avail_in;
      if (input_size > 0) {
        obuf = (output_buffer_max_size > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream, s) : stream->next_out;
        olen = LZ4F_compressUpdate (s->comp.ctx, obuf, output_buffer_max_size, stream->next_in, input_size, NULL);

        if (!LZ4F_isError (olen)) {
          if (input_size + s->comp.input_buffer_size == input_buffer_size) {
            s->comp.input_buffer_size = 0;
          } else {
            s->comp.input_buffer_size += input_size;
            assert (olen == 0);
          }

          stream->next_in += input_size;
          stream->avail_in -= input_size;
        } else {
          HEDLEY_UNREACHABLE();
        }
      } else if (operation == SQUASH_OPERATION_FLUSH) {
        assert (stream->avail_in == 0);
        olen = squash_lz4f_stream_get_output_buffer_size (stream, s);
        obuf = (olen > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream, s) : stream->next_out;
        olen = LZ4F_flush (s->comp.ctx, obuf, olen, NULL);

        s->comp.input_buffer_size = 0;
      } else if (operation == SQUASH_OPERATION_FINISH) {
        assert (stream->avail_in == 0);
        olen = squash_lz4f_stream_get_output_buffer_size (stream, s);
        obuf = (olen > stream->avail_out) ? squash_lz4f_stream_get_output_buffer (stream, s) : stream->next_out;
        olen = LZ4F_compressEnd (s->comp.ctx, obuf, olen, NULL);

        s->comp.input_buffer_size = 0;
      } else if (progress) {
        break;
      } else {
        HEDLEY_UNREACHABLE();
      }

      if (HEDLEY_UNLIKELY(LZ4F_isError (olen))) {
        HEDLEY_UNREACHABLE();
        return squash_error (SQUASH_FAILED);
      } else {
        if (olen != 0) {
          if (obuf == s->comp.output_buffer) {
            s->comp.output_buffer_size = olen;
            break;
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

  if (s->comp.output_buffer_size != 0) {
    const size_t buffer_remaining = s->comp.output_buffer_size - s->comp.output_buffer_pos;
    const size_t cp_size = (buffer_remaining < stream->avail_out) ? buffer_remaining : stream->avail_out;

    memcpy (stream->next_out, s->comp.output_buffer + s->comp.output_buffer_pos, cp_size);
    stream->next_out += cp_size;
    stream->avail_out -= cp_size;
    s->comp.output_buffer_pos += cp_size;

    if (cp_size == buffer_remaining) {
      s->comp.output_buffer_size = 0;
      s->comp.output_buffer_pos = 0;

      progress = true;
    } else {
      return SQUASH_PROCESSING;
    }
  }

  return (stream->avail_in == 0 && s->comp.output_buffer_size == 0) ? SQUASH_OK : SQUASH_PROCESSING;
}

static SquashStatus
squash_lz4f_decompress_stream (SquashStream* stream, SquashOperation operation, SquashLZ4FStream* s) {
  while (stream->avail_in != 0 && stream->avail_out != 0) {
    size_t dst_len = stream->avail_out;
    size_t src_len = stream->avail_in;
    size_t bytes_read = LZ4F_decompress (s->decomp.ctx, stream->next_out, &dst_len, stream->next_in, &src_len, NULL);

    if (LZ4F_isError (bytes_read)) {
      return squash_lz4f_get_status (bytes_read);
    }

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
squash_lz4f_process_stream (SquashStream* stream, SquashOperation operation, void* priv) {
  switch (stream->stream_type) {
    case SQUASH_STREAM_COMPRESS:
      return squash_lz4f_compress_stream (stream, operation, priv);
    case SQUASH_STREAM_DECOMPRESS:
      return squash_lz4f_decompress_stream (stream, operation, priv);
    default:
      HEDLEY_UNREACHABLE();
  }
}

static size_t
squash_lz4f_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  static const LZ4F_preferences_t prefs = {
    { max64KB, blockLinked, contentChecksumEnabled, },
    0, 0,
  };

  const size_t block_size = squash_lz4f_block_size_id_to_size (prefs.frameInfo.blockSizeID);
  const size_t full_blocks = uncompressed_size / block_size;
  const size_t last_block = ((uncompressed_size % block_size) == 0) ? block_size : (uncompressed_size % block_size);
  const size_t block_overhead = 8;

  const size_t res =
    (full_blocks * (block_overhead + block_size)) +
    (last_block == 0 ? 0 : (block_overhead + last_block))
    + 7;

  return res;
}

SquashStatus
squash_plugin_init_lz4f (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("lz4", name) == 0)) {
    impl->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    impl->options = squash_lz4f_options;
    impl->get_max_compressed_size = squash_lz4f_get_max_compressed_size;
    impl->init_stream = squash_lz4f_init_stream;
    impl->priv_size = sizeof(SquashLZ4FStream);
    impl->destroy_stream = squash_lz4f_destroy_stream;
    impl->process_stream = squash_lz4f_process_stream;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
