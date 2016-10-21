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

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"
#include "error_public.h"


typedef struct SquashZstdStream_s {
  SquashStream base_object;
  ZSTD_CStream* cstream;
  ZSTD_DStream* dstream;
} SquashZstdStream;

static void*
squash_zstd_malloc (void* opaque, size_t size) {
  void* address = squash_malloc (size);
  (void)opaque;
  return address;
}

static void
squash_zstd_free (void * opaque, void* address) {
  (void)opaque;
  squash_free (address);
}


SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

enum SquashZstdOptIndex {
  SQUASH_ZSTD_OPT_LEVEL = 0
};

static SquashOptionInfo squash_zstd_options[] = {
  { "level",
    SQUASH_OPTION_TYPE_RANGE_INT,
    .info.range_int = {
      .min = 1,
      .max = 22 },
    .default_value.int_value = 9 },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

static size_t
squash_zstd_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  return ZSTD_compressBound (uncompressed_size);
}

static SquashStatus
squash_zstd_status_from_zstd_error (size_t res) {
  if (!ZSTD_isError (res))
    return SQUASH_OK;

  switch ((ZSTD_ErrorCode) (-(int)(res))) {
    case ZSTD_error_no_error:
      return SQUASH_OK;
    case ZSTD_error_memory_allocation:
      return squash_error (SQUASH_MEMORY);
    case ZSTD_error_dstSize_tooSmall:
      return squash_error (SQUASH_BUFFER_FULL);
    case ZSTD_error_GENERIC:
    case ZSTD_error_prefix_unknown:
    case ZSTD_error_frameParameter_unsupported:
    case ZSTD_error_frameParameter_unsupportedBy32bits:
    case ZSTD_error_init_missing:
    case ZSTD_error_stage_wrong:
    case ZSTD_error_srcSize_wrong:
    case ZSTD_error_corruption_detected:
    case ZSTD_error_tableLog_tooLarge:
    case ZSTD_error_maxSymbolValue_tooLarge:
    case ZSTD_error_maxSymbolValue_tooSmall:
    case ZSTD_error_dictionary_corrupted:
    case ZSTD_error_maxCode:
    case ZSTD_error_compressionParameter_unsupported:
    case ZSTD_error_checksum_wrong:
    case ZSTD_error_dictionary_wrong:
    case ZSTD_error_version_unsupported:
    case ZSTD_error_parameter_unknown:
    default:
      return squash_error (SQUASH_FAILED);
  }

  HEDLEY_UNREACHABLE ();
}

static SquashStatus
squash_zstd_decompress_buffer (SquashCodec* codec,
                               size_t* decompressed_size,
                               uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)],
                               size_t compressed_size,
                               const uint8_t compressed[HEDLEY_ARRAY_PARAM(compressed_size)],
                               SquashOptions* options) {
  *decompressed_size = ZSTD_decompress (decompressed, *decompressed_size, compressed, compressed_size);

  return squash_zstd_status_from_zstd_error (*decompressed_size);
}

static SquashStatus
squash_zstd_compress_buffer (SquashCodec* codec,
                             size_t* compressed_size,
                             uint8_t compressed[HEDLEY_ARRAY_PARAM(*compressed_size)],
                             size_t uncompressed_size,
                             const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                             SquashOptions* options) {
  const int level = squash_options_get_int_at (options, codec, SQUASH_ZSTD_OPT_LEVEL);

  *compressed_size = ZSTD_compress (compressed, *compressed_size, uncompressed, uncompressed_size, level);

  return squash_zstd_status_from_zstd_error (*compressed_size);
}

static void
squash_zstd_stream_destroy (void* s) {
  SquashZstdStream* stream = (SquashZstdStream*) s;

  if (((SquashStream*) s)->stream_type == SQUASH_STREAM_COMPRESS) {
    ZSTD_freeCStream(stream->cstream);
  } else {
    ZSTD_freeDStream(stream->dstream);
  }

  squash_stream_destroy (stream);
}


static SquashStream*
squash_zstd_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);
    
  ZSTD_customMem cMem = { squash_zstd_malloc, squash_zstd_free, NULL };
  
  SquashZstdStream* stream = squash_malloc(sizeof (SquashZstdStream));
  squash_stream_init ((SquashStream*)stream, codec, stream_type, options, squash_zstd_stream_destroy);
  
  if(stream_type == SQUASH_STREAM_COMPRESS) {
    stream->cstream = ZSTD_createCStream_advanced(cMem);
    stream->dstream = NULL;
    
    if(stream->cstream == NULL) {
      squash_free(stream);
      return NULL;
    }
    
    const int level = squash_options_get_int_at (options, codec, SQUASH_ZSTD_OPT_LEVEL);
    size_t const initResult = ZSTD_initCStream(stream->cstream, level);
    if (ZSTD_isError(initResult)) {
      ZSTD_freeCStream(stream->cstream);
      squash_free(stream);
      return NULL;
    }
    
  } else {
    stream->dstream = ZSTD_createDStream_advanced(cMem);
    stream->cstream = NULL;
    
    if(stream->dstream == NULL) {
      squash_free(stream);
      return NULL;
    }
    
    size_t const initResult = ZSTD_initDStream(stream->dstream);
    if (ZSTD_isError(initResult)) {
      ZSTD_freeDStream(stream->dstream);
      squash_free(stream);
      return NULL;
    }
  }
    
  return (SquashStream*) stream;
}
static SquashStatus
squash_zstd_process_stream (SquashStream* ss, SquashOperation operation) {
  SquashZstdStream* stream = (SquashZstdStream*)ss;
  
  ZSTD_inBuffer input = { ss->next_in, ss->avail_in, 0 };
  ZSTD_outBuffer output = { ss->next_out, ss->avail_out, 0 };
  
  if(ss->stream_type == SQUASH_STREAM_COMPRESS) {
    switch (operation) {
      case SQUASH_OPERATION_PROCESS: ;
          size_t hint = ZSTD_compressStream(stream->cstream, &output, &input);
          
          ss->avail_in -= input.pos;
          ss->next_in += input.pos;
          ss->avail_out -= output.pos;
          ss->next_out += output.pos;
          
          if(ZSTD_isError(hint))
            return squash_zstd_status_from_zstd_error(hint);
          
          return  (ss->avail_in != 0) ? SQUASH_PROCESSING : SQUASH_OK;
      case SQUASH_OPERATION_FLUSH: {
          size_t remaining = ZSTD_flushStream(stream->cstream, &output);
          
          ss->avail_out -= output.pos;
          ss->next_out += output.pos;
          
          if(ZSTD_isError(remaining))
            return squash_zstd_status_from_zstd_error(remaining);
          
          return (remaining > 0) ? SQUASH_PROCESSING : SQUASH_OK;
      }
      case SQUASH_OPERATION_FINISH: {
          size_t remaining = ZSTD_endStream(stream->cstream, &output);
          
          ss->avail_out -= output.pos;
          ss->next_out += output.pos;
          
          if(ZSTD_isError(remaining))
            return squash_zstd_status_from_zstd_error(remaining);
          
          return (remaining > 0) ? SQUASH_PROCESSING : SQUASH_OK;
      }
      case SQUASH_OPERATION_TERMINATE:
          HEDLEY_UNREACHABLE();
    }
  } else {
    size_t remaining = ZSTD_decompressStream(stream->dstream, &output, &input);
    
    ss->avail_in -= input.pos;
    ss->next_in += input.pos;
    ss->avail_out -= output.pos;
    ss->next_out += output.pos;
    
    if(ZSTD_isError(remaining))
      return squash_zstd_status_from_zstd_error(remaining);
    
    return (remaining > 0) ? ((ss->avail_in != 0 || ss->avail_out == 0) ? SQUASH_PROCESSING : SQUASH_OK) : SQUASH_END_OF_STREAM;
  }
  return squash_error (SQUASH_FAILED);
}


SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (HEDLEY_LIKELY(strcmp ("zstd", name) == 0)) {
    impl->options = squash_zstd_options;
    impl->get_max_compressed_size = squash_zstd_get_max_compressed_size;
    impl->decompress_buffer = squash_zstd_decompress_buffer;
    impl->compress_buffer_unsafe = squash_zstd_compress_buffer;
    impl->create_stream = squash_zstd_create_stream;
    impl->process_stream = squash_zstd_process_stream;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
