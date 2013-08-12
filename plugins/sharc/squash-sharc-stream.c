/* Copyright (c) 2013 Evan Nemerson <evan@coeus-group.com>
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
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "squash-sharc-stream.h"

typedef enum _SquashSharcStreamState {
  SQUASH_SHARC_STREAM_STATE_IDLE          = 1,
  SQUASH_SHARC_STREAM_STATE_GLOBAL_HEADER = 2,
  SQUASH_SHARC_STREAM_STATE_BUFFERING     = 3,
  SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER  = 4,
  SQUASH_SHARC_STREAM_STATE_BLOCK_DATA    = 5,
  SQUASH_SHARC_STREAM_STATE_FINISHED      = 6,
  SQUASH_SHARC_STREAM_STATE_DETAIL_MASK   = 7,

  SQUASH_SHARC_STREAM_STATE_DEFAULT       = 0,
  SQUASH_SHARC_STREAM_STATE_FLUSHING      = 1 << 3,
  SQUASH_SHARC_STREAM_STATE_FINISHING     = 2 << 3,
  SQUASH_SHARC_STREAM_STATE_BASIC_MASK    = 3 << 3
} SquashSharcStreamState;

struct _SquashSharcStreamPriv {
  SquashSharcStreamState state;

  union {
    SHARC_GENERIC_HEADER global;
    SHARC_BLOCK_HEADER block;
  } header;

  size_t current_progress;

  SHARC_BYTE_BUFFER read_buffer;
  SHARC_BYTE_BUFFER inter_buffer;
  SHARC_BYTE_BUFFER write_buffer;

  sharc_byte inter_data[SHARC_MAX_BUFFER_SIZE];
  sharc_byte* read_data;
  sharc_byte* write_data;
};

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

SquashSharcStreamStatus
squash_sharc_stream_init (SquashSharcStream* stream, SquashSharcStreamType type) {
  SquashSharcStreamPriv* priv;

  assert (stream != NULL);
  assert (type == SQUASH_SHARC_STREAM_COMPRESS || type == SQUASH_SHARC_STREAM_DECOMPRESS);

  stream->next_in = NULL;
  stream->avail_in = 0;
  stream->total_in = 0;

  stream->next_out = NULL;
  stream->avail_out = 0;
  stream->total_out = 0;

  stream->type = type;
  stream->mode = SHARC_MODE_SINGLE_PASS;

  stream->priv = priv = (SquashSharcStreamPriv*) malloc (sizeof (SquashSharcStreamPriv));
  if (priv == NULL)
    return SQUASH_SHARC_STREAM_MEMORY;

  priv->state = SQUASH_SHARC_STREAM_STATE_IDLE;

  priv->write_data = NULL;
  priv->read_data = NULL;

  priv->read_buffer = sharc_createByteBuffer(NULL, 0, 0);
  priv->inter_buffer = sharc_createByteBuffer(priv->inter_data, 0, SHARC_MAX_BUFFER_SIZE);
  priv->write_buffer = sharc_createByteBuffer(NULL, 0, 0);

  return SQUASH_SHARC_STREAM_OK;
}

void
squash_sharc_stream_destroy (SquashSharcStream* stream) {
  SquashSharcStreamPriv* priv;

  assert (stream != NULL);
  assert (stream->priv != NULL);

  priv = stream->priv;
  if (priv->read_data != NULL)
    free (priv->read_data);
  if (priv->write_data != NULL)
    free (priv->write_data);

  free (priv);
  stream->priv = NULL;
}

static void
squash_sharc_stream_init_header (SHARC_GENERIC_HEADER* header) {
  header->magicNumber = SHARC_LITTLE_ENDIAN_32(SHARC_MAGIC_NUMBER);
  header->version[0] = SHARC_MAJOR_VERSION;
  header->version[1] = SHARC_MINOR_VERSION;
  header->version[2] = SHARC_REVISION;
  header->type = SHARC_TYPE_STREAM;
}

static void
squash_sharc_stream_output (SquashSharcStream* stream, size_t size) {
  stream->next_out += size;
  stream->total_out += size;
  stream->avail_out -= size;
  stream->priv->current_progress += size;
}

static void
squash_sharc_stream_input (SquashSharcStream* stream, size_t size) {
  stream->next_in += size;
  stream->total_in += size;
  stream->avail_in -= size;
}

static void
squash_sharc_stream_copy_to_output (SquashSharcStream* stream, const uint8_t* buffer, size_t buffer_length) {
  memcpy (stream->next_out, buffer, buffer_length);
  squash_sharc_stream_output (stream, buffer_length);
}

static SquashSharcStreamStatus
squash_sharc_stream_copy_input (SquashSharcStream* stream, size_t copy_size) {
  SquashSharcStreamPriv* priv = stream->priv;

  if (priv->read_buffer.pointer == NULL) {
    if (priv->read_data == NULL)
      priv->read_data = (uint8_t*) malloc (SHARC_PREFERRED_BUFFER_SIZE);
    if (priv->read_data == NULL)
      return SQUASH_SHARC_STREAM_MEMORY;

    priv->read_buffer.pointer = priv->read_data;
  }

  memcpy (priv->read_buffer.pointer + priv->read_buffer.position, stream->next_in, copy_size);
  priv->read_buffer.position += copy_size;
  squash_sharc_stream_input (stream, copy_size);

  return SQUASH_SHARC_STREAM_OK;
}

static void
squash_sharc_stream_set_state_detailed (SquashSharcStream* stream, SquashSharcStreamState state) {
  SquashSharcStreamPriv* priv = stream->priv;

  priv->state = (priv->state & ~SQUASH_SHARC_STREAM_STATE_DETAIL_MASK) | (SQUASH_SHARC_STREAM_STATE_DETAIL_MASK & state);
  priv->current_progress = 0;
}

static SquashSharcStreamStatus
squash_sharc_stream_compress (SquashSharcStream* stream) {
  SquashSharcStreamPriv* priv = stream->priv;
  uint_fast8_t progress = 0;
  size_t copy_size;
  SHARC_ENCODING_RESULT encoding_result;
  SquashSharcStreamStatus res;

  switch (priv->state & SQUASH_SHARC_STREAM_STATE_DETAIL_MASK) {
    case SQUASH_SHARC_STREAM_STATE_IDLE:
      squash_sharc_stream_init_header (&(priv->header.global));
      squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_GLOBAL_HEADER);
    case SQUASH_SHARC_STREAM_STATE_GLOBAL_HEADER:
      copy_size = MIN(stream->avail_out, sizeof (SHARC_GENERIC_HEADER) - priv->current_progress);
      if (copy_size > 0) {
        squash_sharc_stream_copy_to_output (stream, (uint8_t*) &(priv->header.global) + priv->current_progress, copy_size);
        progress = 1;
      }

      if (priv->current_progress == sizeof (SHARC_GENERIC_HEADER)) {
        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BUFFERING);
      } else {
        return SQUASH_SHARC_STREAM_PROCESSING;
      }
    case SQUASH_SHARC_STREAM_STATE_BUFFERING:
      if ((priv->state & SQUASH_SHARC_STREAM_STATE_FLUSHING) == SQUASH_SHARC_STREAM_STATE_FLUSHING ||
          stream->avail_in + priv->read_buffer.position >= (SHARC_PREFERRED_BUFFER_SIZE)) {
        if (stream->avail_out >= ((SHARC_PREFERRED_BUFFER_SIZE) + sizeof (SHARC_BLOCK_HEADER))) {
          priv->write_buffer = sharc_createByteBuffer(stream->next_out + sizeof (SHARC_BLOCK_HEADER), 0, SHARC_PREFERRED_BUFFER_SIZE);
        } else if (progress) {
          return SQUASH_SHARC_STREAM_PROCESSING;
        } else {
          if (priv->write_data == NULL) {
            priv->write_data = (uint8_t*) malloc (SHARC_PREFERRED_BUFFER_SIZE);
            if (priv->write_data == NULL)
              return SQUASH_SHARC_STREAM_MEMORY;
          }
          priv->write_buffer = sharc_createByteBuffer(priv->write_data, 0, SHARC_PREFERRED_BUFFER_SIZE);
        }

        if (priv->read_buffer.position == 0) {
          copy_size = MIN(stream->avail_in, SHARC_PREFERRED_BUFFER_SIZE);

          priv->read_buffer.pointer = (sharc_byte*) stream->next_in;
          priv->read_buffer.position = copy_size;
          squash_sharc_stream_input (stream, copy_size);
        } else {
          copy_size = MIN(stream->avail_in, ((SHARC_PREFERRED_BUFFER_SIZE) - priv->read_buffer.position));

          res = squash_sharc_stream_copy_input (stream, copy_size);
          if (res != SQUASH_SHARC_STREAM_OK)
            return res;
        }

        priv->read_buffer.size = priv->read_buffer.position;
        priv->read_buffer.position = 0;

        encoding_result = sharc_sharcEncode(&(priv->read_buffer), &(priv->inter_buffer), &(priv->write_buffer), stream->mode);
        priv->header.block.mode = SHARC_LITTLE_ENDIAN_32((const uint32_t) encoding_result.reachableMode);
        priv->header.block.nextBlock = SHARC_LITTLE_ENDIAN_32(encoding_result.out->position);

        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER);
      } else {
        if (stream->avail_in == 0) {
          abort ();
          return SQUASH_SHARC_STREAM_OK;
        } else if (progress) {
          return SQUASH_SHARC_STREAM_PROCESSING;
        }

        copy_size = MIN(stream->avail_in, (SHARC_PREFERRED_BUFFER_SIZE) - priv->read_buffer.position);
        if (copy_size > 0) {
          res = squash_sharc_stream_copy_input (stream, copy_size);
          if (res != SQUASH_SHARC_STREAM_OK)
            return res;
          progress = 1;
        }

        return (stream->avail_in == 0) ? SQUASH_SHARC_STREAM_OK : SQUASH_SHARC_STREAM_PROCESSING;
      }
    case SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER:
      copy_size = MIN(stream->avail_out, sizeof (SHARC_BLOCK_HEADER) - priv->current_progress);
      if (copy_size > 0) {
        squash_sharc_stream_copy_to_output (stream, (uint8_t*) &(priv->header.block) + priv->current_progress, copy_size);
        progress = 1;
      }

      if (priv->current_progress == sizeof (SHARC_BLOCK_HEADER)) {
        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_DATA);
      } else {
        return SQUASH_SHARC_STREAM_PROCESSING;
      }
    case SQUASH_SHARC_STREAM_STATE_BLOCK_DATA:
      if (priv->write_buffer.pointer != stream->next_out) {
        copy_size = MIN(stream->avail_out, priv->write_buffer.position - priv->current_progress);
        if (copy_size > 0) {
          squash_sharc_stream_copy_to_output (stream, (uint8_t*) (priv->write_buffer.pointer + priv->current_progress), copy_size);
          progress = 1;
        }
      } else {
        squash_sharc_stream_output (stream, priv->write_buffer.position);
        progress = 1;
      }

      if (priv->write_buffer.position == priv->current_progress) {
        priv->read_buffer = sharc_createByteBuffer(NULL, 0, 0);
        sharc_rewindByteBuffer (&(priv->inter_buffer));
        priv->write_buffer = sharc_createByteBuffer(NULL, 0, 0);

        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BUFFERING);
        priv->state &= ~SQUASH_SHARC_STREAM_STATE_FLUSHING;
        return SQUASH_SHARC_STREAM_OK;
      } else {
        return SQUASH_SHARC_STREAM_PROCESSING;
      }
    case SQUASH_SHARC_STREAM_STATE_FINISHED:
      return SQUASH_SHARC_STREAM_STATE;
    default:
      return SQUASH_SHARC_STREAM_FAILED;
  }

  if (progress) {
    return (stream->avail_in == 0) ? SQUASH_SHARC_STREAM_OK : SQUASH_SHARC_STREAM_PROCESSING;
  } else {
    return SQUASH_SHARC_STREAM_BUFFER;
  }
}

static SquashSharcStreamStatus
squash_sharc_stream_decompress (SquashSharcStream* stream) {
  SquashSharcStreamPriv* priv = stream->priv;
  uint_fast8_t progress = 0;
  size_t copy_size;

  switch (priv->state & SQUASH_SHARC_STREAM_STATE_DETAIL_MASK) {
    case SQUASH_SHARC_STREAM_STATE_IDLE:
      squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_GLOBAL_HEADER);
    case SQUASH_SHARC_STREAM_STATE_GLOBAL_HEADER:
      copy_size = MIN(stream->avail_in, sizeof (SHARC_GENERIC_HEADER) - priv->current_progress);
      if (copy_size > 0) {
        memcpy (&(priv->header.global) + priv->current_progress, stream->next_in, copy_size);
        squash_sharc_stream_input (stream, copy_size);
        priv->current_progress += copy_size;
        progress = 1;
      }

      if (priv->current_progress == sizeof (SHARC_GENERIC_HEADER)) {
        if (SHARC_LITTLE_ENDIAN_32 (priv->header.global.magicNumber) != SHARC_MAGIC_NUMBER) {
          return SQUASH_SHARC_STREAM_FAILED;
        }

        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER);
      } else {
        break;
      }
    case SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER:
      copy_size = MIN(stream->avail_in, sizeof (SHARC_BLOCK_HEADER) - priv->current_progress);
      if (copy_size > 0) {
        memcpy (&(priv->header.global) + priv->current_progress, stream->next_in, copy_size);
        squash_sharc_stream_input (stream, copy_size);
        priv->current_progress += copy_size;
        progress = 1;
      }

      if (priv->current_progress == sizeof (SHARC_BLOCK_HEADER)) {
        priv->header.block.mode = SHARC_LITTLE_ENDIAN_32(priv->header.block.mode);
        priv->header.block.nextBlock = SHARC_LITTLE_ENDIAN_32(priv->header.block.nextBlock);
        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BUFFERING);
      } else {
        break;
      }
    case SQUASH_SHARC_STREAM_STATE_BUFFERING:
      if (priv->header.block.mode == SHARC_MODE_COPY) {
        copy_size = MIN(stream->avail_in, stream->avail_out);
        if (copy_size > 0) {
          memcpy (stream->next_out, stream->next_in, copy_size);
          squash_sharc_stream_copy_to_output (stream, stream->next_in, copy_size);
          squash_sharc_stream_input (stream, copy_size);
          progress = 1;
        }

        if (priv->current_progress == priv->header.block.nextBlock) {
          squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER);
        }

        break;
      } else {
        if (stream->avail_in + priv->read_buffer.position >= priv->header.block.nextBlock) {
          /* Decompress */
          if (priv->read_buffer.position == 0) {
            priv->read_buffer.pointer = (sharc_byte*) stream->next_in;
            priv->read_buffer.size = priv->header.block.nextBlock;
            priv->read_buffer.position = priv->header.block.nextBlock;
            squash_sharc_stream_input (stream, priv->header.block.nextBlock);
          } else {
            copy_size = MIN(stream->avail_in, priv->header.block.nextBlock - priv->read_buffer.position);

            if (copy_size > 0) {
              memcpy (priv->read_buffer.pointer + priv->read_buffer.position, stream->next_in, copy_size);
              priv->read_buffer.position += copy_size;
              squash_sharc_stream_input (stream, copy_size);

              progress = 1;
            }
          }

          priv->read_buffer.position = 0;

          priv->write_buffer.position = 0;
          priv->write_buffer.size = SHARC_MAX_BUFFER_SIZE;
          if (stream->avail_out >= SHARC_MAX_BUFFER_SIZE) {
            priv->write_buffer.pointer = stream->next_out;
          } else {
            if (priv->write_data == NULL) {
              priv->write_data = (uint8_t*) malloc (SHARC_MAX_BUFFER_SIZE);
              if (priv->write_data == NULL)
                return SQUASH_SHARC_STREAM_MEMORY;
            }

            priv->write_buffer.pointer = priv->write_data;
          }

          if (!sharc_sharcDecode (&(priv->read_buffer), &(priv->inter_buffer), &(priv->write_buffer), priv->header.block.mode)) {
            return SQUASH_SHARC_STREAM_FAILED;
          }

          squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_DATA);
        } else {
          /* Buffer */
          if (priv->read_buffer.size == 0) {
            if (priv->read_data == NULL) {
              priv->read_data = (uint8_t*) malloc (SHARC_MAX_BUFFER_SIZE);
              if (priv->read_data == NULL)
                return SQUASH_SHARC_STREAM_MEMORY;
            }

            priv->read_buffer.pointer = priv->read_data;
            priv->read_buffer.size = priv->header.block.nextBlock;
            priv->read_buffer.position = 0;
          }

          copy_size = MIN(stream->avail_in, priv->header.block.nextBlock - priv->read_buffer.position);

          if (copy_size > 0) {
            memcpy (priv->read_buffer.pointer + priv->read_buffer.position, stream->next_in, copy_size);
            priv->read_buffer.position += copy_size;
            squash_sharc_stream_input (stream, copy_size);

            progress = 1;
          }

          break;
        }
      }
    case SQUASH_SHARC_STREAM_STATE_BLOCK_DATA:
      copy_size = MIN (stream->avail_out, priv->write_buffer.position - priv->current_progress);

      if (copy_size > 0) {
        memcpy (stream->next_out, priv->write_buffer.pointer + priv->current_progress, copy_size);
        squash_sharc_stream_output (stream, copy_size);
        progress = 1;
      }

      if (priv->current_progress == priv->write_buffer.position) {
        squash_sharc_stream_set_state_detailed (stream, SQUASH_SHARC_STREAM_STATE_BLOCK_HEADER);

        priv->read_buffer = sharc_createByteBuffer(NULL, 0, 0);
        priv->inter_buffer = sharc_createByteBuffer(NULL, 0, 0);
        priv->write_buffer = sharc_createByteBuffer(NULL, 0, 0);
      }
      break;
    case SQUASH_SHARC_STREAM_STATE_FINISHED:
      return SQUASH_SHARC_STREAM_STATE;
    default:
      return SQUASH_SHARC_STREAM_FAILED;
  }

  if (progress) {
    return (stream->avail_in == 0) ? SQUASH_SHARC_STREAM_OK : SQUASH_SHARC_STREAM_PROCESSING;
  } else {
    return SQUASH_SHARC_STREAM_BUFFER;
  }
}

SquashSharcStreamStatus
squash_sharc_stream_process (SquashSharcStream* stream) {
  assert (stream != NULL);
  assert (stream->type == SQUASH_SHARC_STREAM_COMPRESS || stream->type == SQUASH_SHARC_STREAM_DECOMPRESS);

  if (stream->type == SQUASH_SHARC_STREAM_COMPRESS) {
    return squash_sharc_stream_compress (stream);
  } else {
    return squash_sharc_stream_decompress (stream);
  }
}

SquashSharcStreamStatus
squash_sharc_stream_flush (SquashSharcStream* stream) {
  SquashSharcStreamPriv* priv = stream->priv;

  if ((priv->state & SQUASH_SHARC_STREAM_STATE_DETAIL_MASK) == SQUASH_SHARC_STREAM_STATE_FINISHED) {
    return SQUASH_SHARC_STREAM_STATE;
  }

  priv->state |= SQUASH_SHARC_STREAM_STATE_FLUSHING;

  return squash_sharc_stream_process (stream);
}

SquashSharcStreamStatus
squash_sharc_stream_finish (SquashSharcStream* stream) {
  SquashSharcStreamPriv* priv = stream->priv;

  if ((priv->state & SQUASH_SHARC_STREAM_STATE_DETAIL_MASK) == SQUASH_SHARC_STREAM_STATE_FINISHED) {
    return SQUASH_SHARC_STREAM_OK;
  }

  priv->state |= SQUASH_SHARC_STREAM_STATE_FLUSHING | SQUASH_SHARC_STREAM_STATE_FINISHING;

  return squash_sharc_stream_process (stream);
}
