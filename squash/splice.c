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

#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200112L

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "internal.h"

#include <string.h>

/**
 * @defgroup Splicing
 * @brief Splicing functions
 *
 * These functions implement a convenient API for copying directly
 * from one file (or file-like stream) to another.
 *
 * @{
 */

/**
 * @brief compress or decompress the contents of one file to another
 *
 * This function will attempt to compress or decompress the contents
 * of one file to another.  It will attempt to use memory-mapped files
 * in order to reduce memory usage and increase performance, and so
 * should be preferred over writing similar code manually.
 *
 * @param fp_in the input *FILE* pointer
 * @param fp_out the output *FILE* pointer
 * @param length number of bytes (uncompressed) to transfer from @a
 *   fp_in to @a fp_out, or 0 to transfer the entire file
 * @param stream_type whether to compress or decompress the data
 * @param codec the name of the codec to use
 * @param ... list of options (with a *NULL* sentinel)
 * @returns @ref SQUASH_OK on success, or a negative error code on
 *   failure
 */
SquashStatus
squash_splice (const char* codec, SquashStreamType stream_type, FILE* fp_out, FILE* fp_in, size_t length, ...) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return squash_error (SQUASH_BAD_PARAM);

  SquashOptions* options = NULL;
  va_list ap;
  va_start (ap, length);
  options = squash_options_newv (codec_i, ap);
  va_end (ap);

  return squash_splice_codec_with_options (codec_i, stream_type, fp_out, fp_in, length, options);
}

/**
 * @brief compress or decompress the contents of one file to another
 *
 * This function will attempt to compress or decompress the contents
 * of one file to another.  It will attempt to use memory-mapped files
 * in order to reduce memory usage and increase performance, and so
 * should be preferred over writing similar code manually.
 *
 * @param fp_in the input *FILE* pointer
 * @param fp_out the output *FILE* pointer
 * @param length number of bytes (uncompressed) to transfer from @a
 *   fp_in to @a fp_out
 * @param stream_type whether to compress or decompress the data
 * @param codec codec to use
 * @param ... list of options (with a *NULL* sentinel)
 * @returns @ref SQUASH_OK on success, or a negative error code on
 *   failure
 */
SquashStatus
squash_splice_codec (SquashCodec* codec, SquashStreamType stream_type, FILE* fp_out, FILE* fp_in, size_t length, ...) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);
  assert (codec != NULL);

  SquashOptions* options = NULL;
  va_list ap;
  va_start (ap, length);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_splice_codec_with_options (codec, stream_type, fp_out, fp_in, length, options);
}

/**
 * @brief compress or decompress the contents of one file to another
 *
 * This function will attempt to compress or decompress the contents
 * of one file to another.  It will attempt to use memory-mapped files
 * in order to reduce memory usage and increase performance, and so
 * should be preferred over writing similar code manually.
 *
 * @param fp_in the input *FILE* pointer
 * @param fp_out the output *FILE* pointer
 * @param length number of bytes (uncompressed) to transfer from @a
 *   fp_in to @a fp_out
 * @param stream_type whether to compress or decompress the data
 * @param codec name of the codec to use
 * @param options options to pass to the codec
 * @returns @ref SQUASH_OK on success, or a negative error code on
 *   failure
 */
SquashStatus
squash_splice_with_options (const char* codec, SquashStreamType stream_type, FILE* fp_out, FILE* fp_in, size_t length, SquashOptions* options) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return squash_error (SQUASH_BAD_PARAM);

  return squash_splice_codec_with_options (codec_i, stream_type, fp_out, fp_in, length, options);
}

static SquashStatus
squash_splice_map (FILE* fp_in, FILE* fp_out, size_t length, SquashStreamType stream_type, SquashCodec* codec, SquashOptions* options) {
  SquashStatus res = SQUASH_FAILED;
  SquashMappedFile mapped_in = squash_mapped_file_empty;
  SquashMappedFile mapped_out = squash_mapped_file_empty;

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    if (!squash_mapped_file_init (&mapped_in, fp_in, length, false))
      goto cleanup;

    const size_t max_output_length = squash_codec_get_max_compressed_size(codec, mapped_in.length);
    if (!squash_mapped_file_init (&mapped_out, fp_out, max_output_length, true))
      goto cleanup;

    res = squash_codec_compress_with_options (codec, &mapped_out.length, mapped_out.data, mapped_in.length, mapped_in.data, options);
    if (res != SQUASH_OK)
      goto cleanup;

    squash_mapped_file_destroy (&mapped_in, true);
    squash_mapped_file_destroy (&mapped_out, true);
  } else {
    if (!squash_mapped_file_init (&mapped_in, fp_in, 0, false))
      goto cleanup;

    const SquashCodecInfo codec_info = squash_codec_get_info (codec);
    const bool knows_uncompressed = ((codec_info & SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) == SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE);

    size_t max_output_length = knows_uncompressed ?
      squash_codec_get_uncompressed_size(codec, mapped_in.length, mapped_in.data) :
      squash_npot (mapped_in.length) << 3;

    do {
      if (!squash_mapped_file_init (&mapped_out, fp_out, max_output_length, true))
        goto cleanup;

      res = squash_codec_decompress_with_options (codec, &mapped_out.length, mapped_out.data, mapped_in.length, mapped_in.data, options);
      if (res == SQUASH_OK) {
        squash_mapped_file_destroy (&mapped_in, true);
        squash_mapped_file_destroy (&mapped_out, true);
      } else {
        max_output_length <<= 1;
      }
    } while (!knows_uncompressed && res == SQUASH_BUFFER_FULL);
  }

 cleanup:

  squash_mapped_file_destroy (&mapped_in, false);
  squash_mapped_file_destroy (&mapped_out, false);

  return res;
}

static SquashStatus
squash_splice_stream (FILE* fp_in,
                      FILE* fp_out,
                      size_t length,
                      SquashStreamType stream_type,
                      SquashCodec* codec,
                      SquashOptions* options) {
  SquashStatus res = SQUASH_FAILED;
  SquashFile* file = NULL;
  size_t remaining = length;
  uint8_t* data = NULL;
  size_t data_length = 0;
#if defined(SQUASH_MMAP_IO)
  bool first_block = true;
  SquashMappedFile map = squash_mapped_file_empty;

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    file = squash_file_steal_codec_with_options (fp_out, codec, options);
    assert (file != NULL);

    while (length == 0 || remaining != 0) {
      const size_t req_size = (length == 0 || remaining > SQUASH_FILE_BUF_SIZE) ? SQUASH_FILE_BUF_SIZE : remaining;
      if (!squash_mapped_file_init_full(&map, fp_in, req_size, true, false)) {
        if (first_block)
          goto nomap;
        else {
          break;
        }
      } else {
        first_block = false;
      }

      res = squash_file_write (file, map.length, map.data);
      if (res != SQUASH_OK)
        goto cleanup;

      if (length != 0)
        remaining -= map.length;
      squash_mapped_file_destroy (&map, true);
    }
  } else { /* stream_type == SQUASH_STREAM_DECOMPRESS */
    file = squash_file_steal_codec_with_options (fp_in, codec, options);
    assert (file != NULL);

    while (length == 0 || remaining > 0) {
      const size_t req_size = (length == 0 || remaining > SQUASH_FILE_BUF_SIZE) ? SQUASH_FILE_BUF_SIZE : remaining;
      if (!squash_mapped_file_init_full(&map, fp_out, req_size, true, true)) {
        if (first_block)
          goto nomap;
        else {
          break;
        }
      } else {
        first_block = false;
      }

      res = squash_file_read (file, &map.length, map.data);
      if (res < 0)
        goto cleanup;

      if (res == SQUASH_END_OF_STREAM) {
        assert (map.length == 0);
        res = SQUASH_OK;
        squash_mapped_file_destroy (&map, true);
        goto cleanup;
      }

      squash_mapped_file_destroy (&map, true);
    }
  }

 nomap:
#endif /* defined(SQUASH_MMAP_IO) */

  if (res != SQUASH_OK) {
    file = squash_file_steal_codec_with_options (codec, (stream_type == SQUASH_STREAM_COMPRESS ? fp_out : fp_in), options);
    if (file == NULL) {
      res = squash_error (SQUASH_FAILED);
      goto cleanup;
    }

    data = malloc (SQUASH_FILE_BUF_SIZE);
    if (data == NULL) {
      res = squash_error (SQUASH_MEMORY);
      goto cleanup;
    }

    if (stream_type == SQUASH_STREAM_COMPRESS) {
      while (length == 0 || remaining != 0) {
        const size_t req_size = (length == 0 || remaining > SQUASH_FILE_BUF_SIZE) ? SQUASH_FILE_BUF_SIZE : remaining;

        data_length = SQUASH_FREAD_UNLOCKED(data, 1, req_size, fp_in);
        if (data_length == 0) {
          res = feof (fp_in) ? SQUASH_OK : squash_error (SQUASH_IO);
          goto cleanup;
        }

        res = squash_file_write (file, data_length, data);
        if (res != SQUASH_OK)
          goto cleanup;

        if (remaining != 0) {
          assert (data_length <= remaining);
          remaining -= data_length;
        }
      }
    } else {
      while (length == 0 || remaining != 0) {
        data_length = (length == 0 || remaining > SQUASH_FILE_BUF_SIZE) ? SQUASH_FILE_BUF_SIZE : remaining;
        res = squash_file_read (file, &data_length, data);
        if (res < 0) {
          break;
        } else if (res == SQUASH_PROCESSING) {
          res = SQUASH_OK;
        }

        if (data_length > 0) {
          size_t bytes_written = SQUASH_FWRITE_UNLOCKED(data, 1, data_length, fp_out);
          /* fprintf (stderr, "%s:%d: bytes_written: %zu\n", __FILE__, __LINE__, data_length); */
          assert (bytes_written == data_length);
          if (bytes_written == 0) {
            res = squash_error (SQUASH_IO);
            break;
          }

          if (remaining != 0) {
            assert (data_length <= remaining);
            remaining -= data_length;
          }
        }

        if (res == SQUASH_END_OF_STREAM) {
          res = SQUASH_OK;
          break;
        }
      }
    }
  }

 cleanup:

  squash_file_free (file, NULL);
#if defined(SQUASH_MMAP_IO)
  squash_mapped_file_destroy (&map, false);
#endif
  free (data);

  return res;
}

static once_flag squash_splice_detect_once = ONCE_FLAG_INIT;
static int squash_splice_try_mmap = 0;

static void
squash_splice_detect_enable (void) {
  char* ev = getenv ("SQUASH_MAP_SPLICE");

  if (ev == NULL || strcmp (ev, "yes") == 0)
    squash_splice_try_mmap = 2;
  else if (strcmp (ev, "always") == 0)
    squash_splice_try_mmap = 3;
  else if (strcmp (ev, "no") == 0)
    squash_splice_try_mmap = 1;
  else
    squash_splice_try_mmap = 2;
}

struct SquashFileSpliceData {
  FILE* fp_in;
  FILE* fp_out;
  size_t length;
  size_t pos;
  SquashStreamType stream_type;
  SquashCodec* codec;
  SquashOptions* options;
};

static SquashStatus
squash_file_splice_read (size_t* data_length,
                         uint8_t data[SQUASH_ARRAY_PARAM(*data_length)],
                         void* user_data) {
  struct SquashFileSpliceData* ctx = (struct SquashFileSpliceData*) user_data;

  size_t requested;

  if (ctx->stream_type == SQUASH_STREAM_COMPRESS && ctx->length != 0) {
    const size_t remaining = ctx->length - ctx->pos;

    if (remaining == 0) {
      *data_length = 0;
      return SQUASH_END_OF_STREAM;
    }

    requested = (*data_length < remaining) ? *data_length : remaining;
  } else {
    requested = *data_length;
    assert (requested != 0);
  }

  const size_t bytes_read = SQUASH_FREAD_UNLOCKED(data, 1, requested, ctx->fp_in);
  *data_length = bytes_read;
  ctx->pos += bytes_read;

  if (bytes_read == 0) {
    return feof (ctx->fp_in) ? SQUASH_END_OF_STREAM : squash_error (SQUASH_IO);
  } else {
    return SQUASH_OK;
  }
}

static SquashStatus
squash_file_splice_write (size_t* data_length,
                          const uint8_t data[SQUASH_ARRAY_PARAM(*data_length)],
                          void* user_data) {
  struct SquashFileSpliceData* ctx = (struct SquashFileSpliceData*) user_data;

  const size_t requested = *data_length;
  *data_length = SQUASH_FWRITE_UNLOCKED(data, 1, requested, ctx->fp_out);

  return (*data_length == requested) ? SQUASH_OK : squash_error (SQUASH_IO);
}

/* I would care more about the absurd name if this were exposed publicly. */
static SquashStatus
squash_file_splice (FILE* fp_in,
                    FILE* fp_out,
                    size_t length,
                    SquashStreamType stream_type,
                    SquashCodec* codec,
                    SquashOptions* options) {
  struct SquashFileSpliceData data = { fp_in, fp_out, length, 0, stream_type, codec, options };

  return codec->impl.splice (codec, options, stream_type, squash_file_splice_read, squash_file_splice_write, &data);
}

/**
 * @brief compress or decompress the contents of one file to another
 *
 * This function will attempt to compress or decompress the contents
 * of one file to another.  It will attempt to use memory-mapped files
 * in order to reduce memory usage and increase performance, and so
 * should be preferred over writing similar code manually.
 *
 * @param fp_in the input *FILE* pointer
 * @param fp_out the output *FILE* pointer
 * @param length number of bytes (uncompressed) to transfer from @a
 *   fp_in to @a fp_out
 * @param stream_type whether to compress or decompress the data
 * @param codec codec to use
 * @param options options to pass to the codec
 * @returns @ref SQUASH_OK on success, or a negative error code on
 *   failure
 */
SquashStatus
squash_splice_codec_with_options (SquashCodec* codec,
                                  SquashStreamType stream_type,
                                  FILE* fp_out,
                                  FILE* fp_in,
                                  size_t length,
                                  SquashOptions* options) {
  SquashStatus res = SQUASH_FAILED;

  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);
  assert (codec != NULL);

  call_once (&squash_splice_detect_once, squash_splice_detect_enable);

  SQUASH_FLOCKFILE(fp_in);
  SQUASH_FLOCKFILE(fp_out);

  if (codec->impl.splice != NULL) {
    res = squash_file_splice (fp_in, fp_out, length, stream_type, codec, options);
  } else {
    if (squash_splice_try_mmap == 3 || (squash_splice_try_mmap == 2 && codec->impl.create_stream == NULL)) {
      res = squash_splice_map (fp_in, fp_out, length, stream_type, codec, options);
    }

    if (res != SQUASH_OK)
      res = squash_splice_stream (fp_in, fp_out, length, stream_type, codec, options);
  }

  SQUASH_FUNLOCKFILE(fp_in);
  SQUASH_FUNLOCKFILE(fp_out);

  return res;
}

/**
 * @}
 */
