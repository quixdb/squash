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

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <endian.h>

#include <squash/squash.h>
#include <snappy-c.h>
#include "crc32.h"

static uint32_t
squash_snappy_framed_mask_checksum (uint32_t x) {
  return ((x >> 15) | (x << 17)) + 0xa282ead8;
}

static uint32_t
squash_snappy_framed_generate_checksum (const uint8_t* data, size_t data_length) {
  uint32_t crc = crc_init ();
  crc = crc_update (crc, data, data_length);
  crc = crc_finalize (crc);
  crc = be32toh (crc);

  return squash_snappy_framed_mask_checksum (crc);
}

enum SquashSnappyFramedState {
  SQUASH_SNAPPY_FRAMED_STATE_INIT,
  SQUASH_SNAPPY_FRAMED_STATE_IDLE,
  SQUASH_SNAPPY_FRAMED_STATE_BUFFERING,
  SQUASH_SNAPPY_FRAMED_STATE_DRAINING
};

enum SquashSnappyFramedChunkType {
  SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_COMPRESSED   = 0x00,
  SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_UNCOMPRESSED = 0x01,
  SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_PADDING      = 0xfe,
  SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_IDENTIFIER   = 0xff
};

#define SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX ((size_t) 65536)
const uint8_t squash_snappy_framed_identifier[] = { 0xff, 0x06, 0x00, 0x00, 0x73, 0x4e, 0x61, 0x50, 0x70, 0x59 };

typedef struct SquashSnappyFramedStream_s {
  SquashStream base_object;

  enum SquashSnappyFramedState state;

  uint8_t* input_buffer;
  size_t input_buffer_length;
  size_t input_buffer_pos;
  size_t input_buffer_size;

  uint8_t* output_buffer;
  size_t output_buffer_length;
  size_t output_buffer_pos;
  size_t output_buffer_size;
} SquashSnappyFramedStream;

static void                      squash_snappy_stream_init      (SquashSnappyFramedStream* stream,
                                                                 SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 SquashOptions* options,
                                                                 SquashDestroyNotify destroy_notify);
static SquashSnappyFramedStream* squash_snappy_stream_new       (SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 SquashOptions* options);
static void                      squash_snappy_stream_destroy   (void* stream);
static void                      squash_snappy_stream_free      (void* stream);

static size_t
squash_snappy_framed_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  size_t res = sizeof(squash_snappy_framed_identifier); // Header
  res += (uncompressed_length / 65536) * (65536 + 8); // Full blocks
  if ((uncompressed_length % 65536) > 0) // Partial blocks
    res += (uncompressed_length % 65536) + 8;

  return res;
}

static size_t
squash_snappy_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return snappy_max_compressed_length (uncompressed_length);
}

static size_t
squash_snappy_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  size_t uncompressed_size = 0;

  snappy_uncompressed_length ((const char*) compressed, compressed_length, &uncompressed_size);

  return uncompressed_size;
}

static SquashStatus
squash_snappy_status (snappy_status status) {
  SquashStatus res;

  switch (status) {
    case SNAPPY_OK:
      res = SQUASH_OK;
      break;
    case SNAPPY_BUFFER_TOO_SMALL:
      res = SQUASH_BUFFER_FULL;
      break;
    default:
      res = SQUASH_FAILED;
      break;
  }

  return res;
}

static SquashStatus
squash_snappy_decompress_buffer (SquashCodec* codec,
                                 uint8_t* decompressed, size_t* decompressed_length,
                                 const uint8_t* compressed, size_t compressed_length,
                                 SquashOptions* options) {
  snappy_status e;

  e = snappy_uncompress ((char*) compressed, compressed_length,
                         (char*) decompressed, decompressed_length);

  return squash_snappy_status (e);
}

static SquashStatus
squash_snappy_compress_buffer (SquashCodec* codec,
                               uint8_t* compressed, size_t* compressed_length,
                               const uint8_t* uncompressed, size_t uncompressed_length,
                               SquashOptions* options) {
  snappy_status e;

  e = snappy_compress ((char*) uncompressed, uncompressed_length,
                       (char*) compressed, compressed_length);

  return squash_snappy_status (e);
}

static void
squash_snappy_framed_stream_init (SquashSnappyFramedStream* stream,
                                  SquashCodec* codec,
                                  SquashStreamType stream_type,
                                  SquashOptions* options,
                                  SquashDestroyNotify destroy_notify) {
  squash_stream_init ((SquashStream*) stream, codec, stream_type, (SquashOptions*) options, destroy_notify);

  stream->state = SQUASH_SNAPPY_FRAMED_STATE_INIT;

  stream->input_buffer = NULL;
  stream->input_buffer_length = 0;
  stream->input_buffer_pos = 0;
  stream->input_buffer_size = 0;

  stream->output_buffer = NULL;
  stream->output_buffer_length = 0;
  stream->output_buffer_pos = 0;
  stream->output_buffer_size = 0;
}

static void
squash_snappy_framed_stream_destroy (void* stream) {
  SquashSnappyFramedStream* s = (SquashSnappyFramedStream*) stream;

  if (s->input_buffer != NULL)
    free (s->input_buffer);
  if (s->output_buffer != NULL)
    free (s->output_buffer);

  squash_stream_destroy (stream);
}

static void
squash_snappy_framed_stream_free (void* stream) {
  squash_snappy_framed_stream_destroy (stream);
  free (stream);
}

static SquashSnappyFramedStream*
squash_snappy_framed_stream_new (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  SquashSnappyFramedStream* stream;

  assert (codec != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  stream = (SquashSnappyFramedStream*) malloc (sizeof (SquashSnappyFramedStream));
  squash_snappy_framed_stream_init (stream, codec, stream_type, options, squash_snappy_framed_stream_free);

  return stream;
}

static SquashStream*
squash_snappy_framed_create_stream (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options) {
  return (SquashStream*) squash_snappy_framed_stream_new (codec, stream_type, (SquashOptions*) options);
}

static size_t
squash_snappy_framed_skip (SquashSnappyFramedStream* s, const size_t desired) {
  SquashStream* stream = (SquashStream*) s;

  const size_t cp_size = stream->avail_in < desired ? stream->avail_in : desired;
  if (cp_size != 0) {
    s->input_buffer_length += cp_size;
    stream->next_in += cp_size;
    stream->avail_in -= cp_size;

    s->state = SQUASH_SNAPPY_FRAMED_STATE_BUFFERING;
  }

  return cp_size;
}

static size_t
squash_snappy_framed_read_to_buffer (SquashSnappyFramedStream* s, const size_t desired) {
  SquashStream* stream = (SquashStream*) s;

  const size_t cp_size = stream->avail_in < desired ? stream->avail_in : desired;
  if (cp_size != 0) {
    memcpy (s->input_buffer + s->input_buffer_length, stream->next_in, cp_size);
    s->input_buffer_length += cp_size;
    stream->next_in += cp_size;
    stream->avail_in -= cp_size;

    s->state = SQUASH_SNAPPY_FRAMED_STATE_BUFFERING;
  }

  return cp_size;
}

static size_t
squash_snappy_framed_header_get_chunk_size (const uint8_t header[4]) {
  return
    (header[1] <<  0) |
    (header[2] <<  8) |
    (header[3] << 16);
}

static bool
squash_snappy_framed_header_skippable (const uint8_t header[4]) {
  return header[0] >= 0x80 && header[0] <= 0xfe;
}

#define SQUASH_SNAPPY_FRAMED_MAX_CHUNK_SIZE ((size_t) 16777211)

static size_t
squash_snappy_framed_buffer (SquashSnappyFramedStream* s) {
  SquashStream* stream = (SquashStream*) s;

  if (stream->stream_type == SQUASH_STREAM_COMPRESS) {
    if (s->input_buffer == NULL) {
      s->input_buffer_size = SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX;
      s->input_buffer = malloc (s->input_buffer_size);
    }
    return squash_snappy_framed_read_to_buffer(s, SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX - s->input_buffer_length);
  } else {
    if (s->input_buffer == NULL) {
      s->input_buffer_size = SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX + 8;
      s->input_buffer = malloc (s->input_buffer_size);
    }

    size_t bytes_read = 0;
    size_t chunk_size;

    if (s->input_buffer_length < 4) {
      bytes_read += squash_snappy_framed_read_to_buffer (s, 4 - s->input_buffer_length);
      if (s->input_buffer_length != 4)
        return bytes_read;
    }

    chunk_size = squash_snappy_framed_header_get_chunk_size (s->input_buffer);

    if (s->input_buffer_size < chunk_size + 4 &&
        !squash_snappy_framed_header_skippable (s->input_buffer)) {
      if (chunk_size > SQUASH_SNAPPY_FRAMED_MAX_CHUNK_SIZE)
        return SQUASH_SNAPPY_FRAMED_MAX_CHUNK_SIZE + 1;
      s->input_buffer_size = chunk_size + 4;
      s->input_buffer = realloc (s->input_buffer, s->input_buffer_size);
    }

    const size_t remaining = (chunk_size + 4) - s->input_buffer_length;
    const size_t cp_size = remaining;

    if (squash_snappy_framed_header_skippable (s->input_buffer)) {
      bytes_read += squash_snappy_framed_skip (s, remaining);
    } else {
      bytes_read += squash_snappy_framed_read_to_buffer (s, remaining);
    }

    return bytes_read;
  }
}

static size_t
squash_snappy_framed_flush (SquashSnappyFramedStream* s) {
  assert (s->state == SQUASH_SNAPPY_FRAMED_STATE_DRAINING);

  SquashStream* stream = (SquashStream*) s;

  const size_t remaining = s->output_buffer_length - s->output_buffer_pos;
  const size_t cp_size = (remaining < stream->avail_out) ? remaining : stream->avail_out;

  if (cp_size == 0)
    return 0;

  memcpy (stream->next_out, s->output_buffer + s->output_buffer_pos, cp_size);
  stream->next_out += cp_size;
  stream->avail_out -= cp_size;
  s->output_buffer_pos += cp_size;

  if (s->output_buffer_pos == s->output_buffer_length) {
    s->output_buffer_length = 0;
    s->output_buffer_pos = 0;
    s->state = SQUASH_SNAPPY_FRAMED_STATE_IDLE;
  }

  return cp_size;
}

static bool
squash_snappy_framed_handle_chunk (SquashSnappyFramedStream* s) {
  SquashStream* stream = (SquashStream*) s;

  const uint8_t* compressed = NULL;
  size_t compressed_length = 0;

  if (s->input_buffer_length != 0) {
    compressed = s->input_buffer;
  } else {
    compressed = stream->next_in;
  }
  compressed_length = squash_snappy_framed_header_get_chunk_size (compressed);

  if (compressed[0] == SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_IDENTIFIER) {
    if (compressed_length + 4 == sizeof(squash_snappy_framed_identifier) &&
        memcmp (compressed, squash_snappy_framed_identifier, sizeof(squash_snappy_framed_identifier)) == 0) {
    } else {
      return false;
    }
  } else if (squash_snappy_framed_header_skippable (compressed)) {
    // Skip
  } else if (compressed[0] == SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_COMPRESSED ||
             compressed[0] == SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_UNCOMPRESSED) {
    uint8_t* decompressed;
    size_t decompressed_length;

    if (compressed[0] == SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_COMPRESSED) {
      snappy_uncompressed_length (compressed + 8, compressed_length, &decompressed_length);
    } else {
      decompressed_length = compressed_length - 4;
    }

    if (decompressed_length > SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX)
      return false;

    if (stream->avail_out >= decompressed_length) {
      decompressed = stream->next_out;
    } else {
      if (s->output_buffer == NULL) {
        s->output_buffer_size = SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX;
        s->output_buffer = malloc (s->output_buffer_size);
      }
      decompressed = s->output_buffer;
    }

    if (compressed[0] == SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_COMPRESSED) {
      snappy_status e =
        snappy_uncompress ((char*) compressed + 8, compressed_length - 4,
                           (char*) decompressed, &decompressed_length);

      if (e != SNAPPY_OK)
        return false;

      uint32_t crc = 0;
      crc = (crc << 8) | compressed[7];
      crc = (crc << 8) | compressed[6];
      crc = (crc << 8) | compressed[5];
      crc = (crc << 8) | compressed[4];

      if (crc != squash_snappy_framed_generate_checksum(decompressed, decompressed_length)) {
        return false;
      }
    } else {
      memcpy (decompressed, compressed + 8, compressed_length - 4);
    }

    if (decompressed == stream->next_out) {
      stream->next_out += decompressed_length;
      stream->avail_out -= decompressed_length;
    } else {
      s->output_buffer_length = decompressed_length;
      s->output_buffer_pos = 0;
      s->state = SQUASH_SNAPPY_FRAMED_STATE_DRAINING;
      return true;
    }
  } else {
    return false;
  }

  if (compressed == stream->next_in) {
    stream->next_in += compressed_length + 4;
    stream->avail_in -= compressed_length + 4;
  } else {
    s->input_buffer_length = 0;
    s->input_buffer_pos = 0;
  }

  s->state = SQUASH_SNAPPY_FRAMED_STATE_IDLE;

  return true;
}

static void
squash_snappy_framed_compress_chunk (SquashSnappyFramedStream* s) {
  SquashStream* stream = (SquashStream*) s;

  assert (s->state == SQUASH_SNAPPY_FRAMED_STATE_IDLE ||
          s->state == SQUASH_SNAPPY_FRAMED_STATE_BUFFERING);

  uint8_t* compressed;
  size_t compressed_length;
  const uint8_t* uncompressed;
  size_t uncompressed_length;

  if (s->input_buffer_length != 0) {
    squash_snappy_framed_buffer (s);
    uncompressed = s->input_buffer;
    uncompressed_length = s->input_buffer_length;
  } else {
    uncompressed = stream->next_in;
    uncompressed_length = (stream->avail_in < SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX) ? stream->avail_in : SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX;
  }

  if (uncompressed_length == 0)
    return;

  compressed_length = snappy_max_compressed_length (SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX);

  if (stream->avail_out >= (SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX + 8)) {
    compressed = stream->next_out;
  } else {
    if (s->output_buffer == NULL) {
      s->output_buffer = malloc (compressed_length + 8);
      s->output_buffer_size = compressed_length + 8;
    }
    compressed = s->output_buffer;
  }

  uint32_t crc = squash_snappy_framed_generate_checksum(uncompressed, uncompressed_length);

  snappy_status e =
    snappy_compress ((char*) uncompressed, uncompressed_length,
                     (char*) compressed + 8, &compressed_length);

  compressed[4] = (crc >>  0) & 0xff;
  compressed[5] = (crc >>  8) & 0xff;
  compressed[6] = (crc >> 16) & 0xff;
  compressed[7] = (crc >> 24) & 0xff;

  if (e != SNAPPY_OK || compressed_length >= uncompressed_length) {
    compressed[0] = SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_UNCOMPRESSED;
    compressed_length = uncompressed_length;
    memcpy (compressed + 8, uncompressed, uncompressed_length);
  } else {
    compressed[0] = SQUASH_SNAPPY_FRAMED_CHUNK_TYPE_COMPRESSED;
  }

  compressed[1] = ((compressed_length + 4) >>  0) & 0xff;
  compressed[2] = ((compressed_length + 4) >>  8) & 0xff;
  compressed[3] = ((compressed_length + 4) >> 16) & 0xff;

  if (s->input_buffer_length != 0) {
    assert (s->input_buffer == uncompressed);
    s->input_buffer_length = 0;
    s->input_buffer_pos = 0;
  } else {
    assert (stream->next_in == uncompressed);
    stream->next_in += uncompressed_length;
    stream->avail_in -= uncompressed_length;
  }

  assert (compressed_length != 0);
  if (compressed == s->output_buffer) {
    s->output_buffer_length = compressed_length + 8;
    s->output_buffer_pos = 0;

    s->state = SQUASH_SNAPPY_FRAMED_STATE_DRAINING;
  } else {
    assert (stream->next_out == compressed);

    stream->next_out += compressed_length + 8;
    stream->avail_out -= compressed_length + 8;

    s->state = SQUASH_SNAPPY_FRAMED_STATE_IDLE;
  }
}

static SquashStatus
squash_snappy_framed_compress_stream (SquashStream* stream, SquashOperation operation) {
  SquashSnappyFramedStream* s = (SquashSnappyFramedStream*) stream;

  while (true) {
    if (s->state == SQUASH_SNAPPY_FRAMED_STATE_DRAINING) {
      if (squash_snappy_framed_flush (s) == 0)
        return SQUASH_PROCESSING;
    } else if (s->state == SQUASH_SNAPPY_FRAMED_STATE_INIT) {
      const uint8_t identifier[] = { 0xff, 0x06, 0x00, 0x00, 0x73, 0x4e, 0x61, 0x50, 0x70, 0x59 };
      if (stream->avail_out >= sizeof(identifier)) {
        memcpy (stream->next_out, identifier, sizeof(identifier));
        stream->next_out += sizeof(identifier);
        stream->avail_out -= sizeof(identifier);
        s->state = SQUASH_SNAPPY_FRAMED_STATE_IDLE;
      } else {
        if (s->output_buffer == NULL) {
          s->output_buffer_size = snappy_max_compressed_length (SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX + 8);
          s->output_buffer = malloc (s->output_buffer_size);
        }
        memcpy (s->output_buffer, identifier, sizeof(identifier));
        s->output_buffer_length = sizeof(identifier);
        s->output_buffer_pos = 0;
        s->state = SQUASH_SNAPPY_FRAMED_STATE_DRAINING;
      }
    } else {
      if ((s->input_buffer_length != 0) ||
          (operation == SQUASH_OPERATION_PROCESS && stream->avail_in < SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX)) {
        squash_snappy_framed_buffer (s);
      }

      if ((operation != SQUASH_OPERATION_PROCESS) ||
          (s->input_buffer_length == SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX) ||
          (stream->avail_in >= SQUASH_SNAPPY_FRAMED_UNCOMPRESSED_MAX)) {
        squash_snappy_framed_compress_chunk (s);
      }
    }

    if (s->state == SQUASH_SNAPPY_FRAMED_STATE_IDLE ||
        s->state == SQUASH_SNAPPY_FRAMED_STATE_BUFFERING) {
      if (stream->avail_in == 0)
        return SQUASH_OK;
    } else if (s->state == SQUASH_SNAPPY_FRAMED_STATE_DRAINING) {
      return SQUASH_PROCESSING;
    }
  }

  return SQUASH_FAILED;
}

static SquashStatus
squash_snappy_framed_decompress_stream (SquashStream* stream, SquashOperation operation) {
  SquashSnappyFramedStream* s = (SquashSnappyFramedStream*) stream;

  if (s->state == SQUASH_SNAPPY_FRAMED_STATE_INIT)
    s->state = SQUASH_SNAPPY_FRAMED_STATE_IDLE;

  while (true) {
    if (s->state == SQUASH_SNAPPY_FRAMED_STATE_IDLE) {
      if (stream->avail_in >= 4) {
        const size_t chunk_size = squash_snappy_framed_header_get_chunk_size (stream->next_in);
        if (stream->avail_in >= 4 + chunk_size) {
          if (!squash_snappy_framed_handle_chunk (s))
            return SQUASH_FAILED;
        } else {
          squash_snappy_framed_buffer (s);
        }
      }
    }

    if (s->state == SQUASH_SNAPPY_FRAMED_STATE_BUFFERING &&
        stream->avail_in != 0) {
      squash_snappy_framed_buffer (s);
      if (s->input_buffer_length >= 4) {
        const size_t chunk_size = squash_snappy_framed_header_get_chunk_size (s->input_buffer);
        if (s->input_buffer_length >= 4 + chunk_size) {
          if (!squash_snappy_framed_handle_chunk (s))
            return SQUASH_FAILED;
        }
      }
    }

    if (s->state == SQUASH_SNAPPY_FRAMED_STATE_IDLE ||
        s->state == SQUASH_SNAPPY_FRAMED_STATE_BUFFERING) {
      if (stream->avail_in == 0)
        return SQUASH_OK;
    } else if (s->state == SQUASH_SNAPPY_FRAMED_STATE_DRAINING) {
      if (squash_snappy_framed_flush (s) == 0)
        return SQUASH_PROCESSING;
    }
  }

  return SQUASH_FAILED;
}

static SquashStatus
squash_snappy_framed_process_stream (SquashStream* stream, SquashOperation operation) {
  if (stream->stream_type == SQUASH_STREAM_COMPRESS)
    return squash_snappy_framed_compress_stream (stream, operation);
  else
    return squash_snappy_framed_decompress_stream (stream, operation);
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("snappy", name) == 0) {
    funcs->get_uncompressed_size = squash_snappy_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_snappy_get_max_compressed_size;
    funcs->decompress_buffer = squash_snappy_decompress_buffer;
    funcs->compress_buffer = squash_snappy_compress_buffer;
  } else if (strcmp ("snappy-framed", name) == 0) {
    funcs->info = SQUASH_CODEC_INFO_CAN_FLUSH;
    funcs->get_max_compressed_size = squash_snappy_framed_get_max_compressed_size;
    funcs->create_stream = squash_snappy_framed_create_stream;
    funcs->process_stream = squash_snappy_framed_process_stream;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}
