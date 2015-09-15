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

#include "internal.h"

#ifndef SQUASH_FILE_BUF_SIZE
#  define SQUASH_FILE_BUF_SIZE ((size_t) (1024 * 1024))
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

/**
 * @cond INTERNAL
 */
struct _SquashFile {
  FILE* fp;
  bool eof;
  SquashStream* stream;
  SquashCodec* codec;
  SquashOptions* options;
  uint8_t buf[SQUASH_FILE_BUF_SIZE];
};
/**
 * @endcond
 */

/**
 * @defgroup SquashFile SquashFile
 * @brief stdio-like API and utilities
 *
 * These functions provide an API which should be familiar for those
 * used to dealing with the standard I/O functions.
 *
 * Additionally, a splice function family is provided which is
 * inspired by the Linux-specific splice(2) function.
 *
 * @{
 */

/**
 * @brief Open a file
 *
 * The @a mode parameter will be passed through to @a fopen, so the
 * value must valid.  Note that Squash may attempt to use @a mmap
 * regardless of whether the *m* flag is passed.
 *
 * The file is always assumed to be compressed—calling @ref
 * squash_file_write will always compress, and calling @ref
 * squash_file_read will always decompress.  Note, however, that you
 * cannot mix reading and writing to the same file as you can with a
 * standard *FILE*.
 *
 * @note Error handling for this function is somewhat limited, and it
 * may be difficult to determine the exact nature of problems such as
 * an invalid codec, where errno is not set.  If this is unacceptable
 * you should call @ref squash_get_codec and @ref squash_options_parse
 * yourself and pass the results to @ref
 * squash_file_open_codec_with_options (which will only fail due to
 * the underlying *fopen* failing).
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open_codec
 * @see squash_file_open_with_options
 * @see squash_file_open_codec_with_options
 */
SquashFile*
squash_file_open (const char* filename, const char* mode, const char* codec, ...) {
  va_list ap;
  SquashOptions* options;
  SquashCodec* codec_i;

  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return NULL;

  va_start (ap, codec);
  options = squash_options_newv (codec_i, ap);
  va_end (ap);

  return squash_file_open_codec_with_options (filename, mode, codec_i, options);
}

/**
 * @brief Open a file using a codec instance
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open
 */
SquashFile*
squash_file_open_codec (const char* filename, const char* mode, SquashCodec* codec, ...) {
  va_list ap;
  SquashOptions* options;

  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  va_start (ap, codec);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_file_open_codec_with_options (filename, mode, codec, options);
}

/**
 * @brief Open a file with the specified options
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open
 */
SquashFile*
squash_file_open_with_options (const char* filename, const char* mode, const char* codec, SquashOptions* options) {
  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return NULL;

  return squash_file_open_codec_with_options (filename, mode, codec_i, options);
}

/**
 * @brief Open a file using a codec instance with the specified options
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open
 */
SquashFile*
squash_file_open_codec_with_options (const char* filename, const char* mode, SquashCodec* codec, SquashOptions* options) {
  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  FILE* fp = fopen (filename, mode);
  if (fp == NULL)
    return NULL;

  return squash_file_steal_codec_with_options (fp, codec, options);
}


/**
 * @brief Open an existing stdio file
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal_codec
 * @see squash_file_steal_with_options
 * @see squash_file_steal_codec_with_options
 */
SquashFile*
squash_file_steal (FILE* fp, const char* codec, ...) {
  va_list ap;
  SquashOptions* options;

  assert (fp != NULL);
  assert (codec != NULL);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return NULL;
  va_start (ap, codec);
  options = squash_options_newv (codec_i, ap);
  va_end (ap);

  return squash_file_steal_codec_with_options (fp, codec_i, options);
}

/**
 * @brief Open an existing stdio file using a codec instance
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal_codec
 * @see squash_file_steal_with_options
 * @see squash_file_steal_codec_with_options
 */
SquashFile*
squash_file_steal_codec (FILE* fp, SquashCodec* codec, ...) {
  va_list ap;
  SquashOptions* options;

  assert (fp != NULL);
  assert (codec != NULL);

  va_start (ap, codec);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_file_steal_codec_with_options (fp, codec, options);
}

/**
 * @brief Open an existing stdio file with the specified options
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal_codec
 * @see squash_file_steal_with_options
 * @see squash_file_steal_codec_with_options
 */
SquashFile*
squash_file_steal_with_options (FILE* fp, const char* codec, SquashOptions* options) {
  assert (fp != NULL);
  assert (codec != NULL);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return NULL;

  return squash_file_steal_codec_with_options (fp, codec_i, options);
}

/**
 * @brief Open an existing stdio file using a codec instance with the specified options
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal_codec
 * @see squash_file_steal_with_options
 * @see squash_file_steal_codec_with_options
 */
SquashFile*
squash_file_steal_codec_with_options (FILE* fp, SquashCodec* codec, SquashOptions* options) {
  assert (fp != NULL);
  assert (codec != NULL);

  SquashFile* file = malloc (sizeof (SquashFile));
  if (file == NULL)
    return NULL;

  file->fp = fp;
  file->eof = false;
  file->stream = NULL;
  file->codec = codec;
  file->options = (options != NULL) ? squash_object_ref (options) : NULL;

  return file;
}

/**
 * @brief Read from a compressed file
 *
 * Attempt to read @a decompressed_length bytes of decompressed data
 * into the @a decompressed buffer.  The number of bytes of compressed
 * data read from the input file may be **significantly** more, or less,
 * than @a decompressed_length.
 *
 * The number of decompressed bytes successfully read from the file
 * will be stored in @a decompressed_read after this function is
 * execute.  This value will never be greater than @a
 * decompressed_length, but it may be less if there was an error or
 * the end of the input file was reached.
 *
 * @param file the file to read from
 * @param decompressed_length number of bytes to attempt to write to @a decompressed
 * @param decompressed buffer to write the decompressed data to
 * @return the result of the operation
 * @retval SQUASH_OK successfully read some data
 * @retval SQUASH_END_OF_STREAM the end of the file was reached
 */
SquashStatus
squash_file_read (SquashFile* file,
                  size_t* decompressed_length,
                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)]) {
  SquashStatus res = SQUASH_OK;

  assert (file != NULL);
  assert (decompressed_length != NULL);
  assert (decompressed != NULL);

  if (file->stream == NULL) {
    file->stream = squash_codec_create_stream_with_options (file->codec, SQUASH_STREAM_DECOMPRESS, file->options);
    if (file->stream == NULL)
      return squash_error (SQUASH_FAILED);
  }

  assert (file->stream->next_out == NULL);
  assert (file->stream->avail_out == 0);

  file->stream->next_out = decompressed;
  file->stream->avail_out = *decompressed_length;

  while (file->stream->avail_out != 0 && res > 0) {
    if (file->stream->avail_in == 0 && !file->eof) {
      /* Input buffer is empty, fill it back up */
      file->stream->next_in = file->buf;
      file->stream->avail_in = fread (file->buf, 1, SQUASH_FILE_BUF_SIZE, file->fp);
      if (file->stream->avail_in == 0 && feof (file->fp)) {
        file->stream->next_in = NULL;
        file->stream->avail_in = 0;
        file->eof = true;
      }
    }

    if (file->eof) {
      res = squash_stream_process (file->stream);
      if (res == SQUASH_END_OF_STREAM)
        file->eof = true;
    } else {
      res = squash_stream_finish (file->stream);
      if (res == SQUASH_OK || res == SQUASH_END_OF_STREAM)
        file->eof = true;
    }

    if (file->eof)
      break;
  }

  if (res > 0) {
    if (file->stream->avail_out != 0) {
      assert (file->eof);
    }

    *decompressed_length = *decompressed_length - file->stream->avail_out;
    file->stream->next_out = NULL;
    file->stream->avail_out = 0;
  }

  return res;
}

static SquashStatus
squash_file_write_internal (SquashFile* file,
                            size_t uncompressed_length,
                            const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                            SquashOperation operation) {
  SquashStatus res;

  assert (file != NULL);

  if (file->stream == NULL) {
    file->stream = squash_codec_create_stream_with_options (file->codec, SQUASH_STREAM_COMPRESS, file->options);
    if (file->stream == NULL)
      return squash_error (SQUASH_FAILED);
  }

  assert (file->stream->next_in == NULL);
  assert (file->stream->avail_in == 0);
  assert (file->stream->next_out == NULL);
  assert (file->stream->avail_out == 0);

  file->stream->next_in = uncompressed;
  file->stream->avail_in = uncompressed_length;

  file->stream->next_in = uncompressed;
  file->stream->avail_in = uncompressed_length;

  do {
    file->stream->next_out = file->buf;
    file->stream->avail_out = SQUASH_FILE_BUF_SIZE;

    switch (operation) {
      case SQUASH_OPERATION_PROCESS:
        res = squash_stream_process (file->stream);
        break;
      case SQUASH_OPERATION_FLUSH:
        res = squash_stream_flush (file->stream);
        break;
      case SQUASH_OPERATION_FINISH:
        res = squash_stream_finish (file->stream);
        break;
      case SQUASH_OPERATION_TERMINATE:
        squash_assert_unreachable ();
        break;
    }

    if (res > 0 && file->stream->avail_out != SQUASH_FILE_BUF_SIZE) {
      size_t bytes_written = fwrite (file->buf, 1, SQUASH_FILE_BUF_SIZE - file->stream->avail_out, file->fp);
      if (bytes_written != SQUASH_FILE_BUF_SIZE - file->stream->avail_out)
        return SQUASH_IO;
    }
  } while (res == SQUASH_PROCESSING);

  file->stream->next_in = NULL;
  file->stream->avail_in = 0;
  file->stream->next_out = NULL;
  file->stream->avail_out = 0;

  return res;
}

/**
 * @brief Write data to a compressed file
 *
 * Attempt to write the compressed equivalent of @a uncompressed to a
 * file.  The number of bytes of compressed data written to the output
 * file may be **significantly** more, or less, than the @a
 * uncompressed_length.
 *
 * @note It is likely the compressed data will actually be buffered,
 * not immediately written to the file.  For codecs which support
 * flushing you can use @ref squash_file_flush to flush the data,
 * otherwise the data may not be written until @ref squash_file_close
 * or @ref squash_file_free is called.
 *
 * @param file file to write to
 * @param uncompressed_length number of bytes of uncompressed data in
 *   @a uncompressed to attempt to write
 * @param uncompressed data to write
 * @return result of the operation
 */
SquashStatus
squash_file_write (SquashFile* file,
                   size_t uncompressed_length,
                   const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)]) {
  return squash_file_write_internal (file, uncompressed_length, uncompressed, SQUASH_OPERATION_PROCESS);
}

/**
 * @brief immediately write any buffered data to a file
 *
 * @note This function only works for codecs which support flushing
 * (see the @ref SQUASH_CODEC_INFO_CAN_FLUSH flag in the return value
 * of @ref squash_codec_get_info).
 *
 * @param file file to flush
 * @returns *TRUE* if flushing succeeeded, *FALSE* if flushing is not
 *   supported or there was another error.
 */
SquashStatus
squash_file_flush (SquashFile* file) {
  return squash_file_write_internal (file, 0, NULL, SQUASH_OPERATION_FINISH);
}

/**
 * @brief Determine whether the file has reached the end of file
 *
 * @param file file to check
 * @returns *TRUE* if EOF was reached, *FALSE* otherwise
 */
bool
squash_file_eof (SquashFile* file) {
  return file->eof && (file->stream->avail_in == 0);
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
 *   fp_in to @a fp_out, or 0 to transfer the entire file
 * @param stream_type whether to compress or decompress the data
 * @param codec the name of the codec to use
 * @param ... list of options (with a *NULL* sentinel)
 * @returns @ref SQUASH_OK on success, or a negative error code on
 *   failure
 */
SquashStatus
squash_splice (FILE* fp_in, FILE* fp_out, size_t length, SquashStreamType stream_type, const char* codec, ...) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return squash_error (SQUASH_BAD_PARAM);

  SquashOptions* options = NULL;
  va_list ap;
  va_start (ap, codec);
  options = squash_options_newv (codec_i, ap);
  va_end (ap);

  return squash_splice_codec_with_options (fp_in, fp_out, length, stream_type, codec_i, options);
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
squash_splice_codec (FILE* fp_in, FILE* fp_out, size_t length, SquashStreamType stream_type, SquashCodec* codec, ...) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);
  assert (codec != NULL);

  SquashOptions* options = NULL;
  va_list ap;
  va_start (ap, codec);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_splice_codec_with_options (fp_in, fp_out, length, stream_type, codec, options);
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
squash_splice_with_options (FILE* fp_in, FILE* fp_out, size_t length, SquashStreamType stream_type, const char* codec, SquashOptions* options) {
  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);

  SquashCodec* codec_i = squash_get_codec (codec);
  if (codec_i == NULL)
    return squash_error (SQUASH_BAD_PARAM);

  return squash_splice_codec_with_options (fp_in, fp_out, length, stream_type, codec_i, options);
}

#if defined(__GNUC__)
__attribute__ ((__const__))
#endif
static size_t
squash_npot (size_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
  v |= v >> 32;
#endif
  v++;
  return v;
}

/**
 * @cond INTERNAL
 */
typedef struct SquashMappedFile_s {
  uint8_t* data;
  size_t length;
  size_t map_length;
  size_t window_offset;
  FILE* fp;
  bool writable;
} SquashMappedFile;

static bool
squash_mapped_file_init (SquashMappedFile* mapped, FILE* fp, size_t length, bool writable) {
  assert (mapped != NULL);
  assert (fp != NULL);

  if (mapped->data != MAP_FAILED)
    munmap (mapped->data - mapped->window_offset, mapped->length + mapped->window_offset);

  int fd = fileno (fp);
  if (fd == -1)
    return false;

  int ires;
  struct stat fp_stat;

  ires = fstat (fd, &fp_stat);
  if (ires == -1 || !S_ISREG(fp_stat.st_mode) || (!writable && fp_stat.st_size == 0))
    return false;

  off_t offset = ftello (fp);
  if (offset < 0)
    return false;

  if (writable) {
    ires = ftruncate (fd, offset + (off_t) length);
    if (ires == -1)
      return false;
  } else {
    if (length == 0) {
      length = fp_stat.st_size - (size_t) offset;
    } else if (((off_t) length + offset) > fp_stat.st_size) {
      return false;
    }
  }
  mapped->length = length;

  const size_t page_size = (size_t) sysconf (_SC_PAGE_SIZE);
  mapped->window_offset = (size_t) offset % page_size;
  mapped->map_length = length + mapped->window_offset;

  mapped->data = mmap (NULL, mapped->map_length, writable ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, fd, offset - mapped->window_offset);
  if (mapped->data == MAP_FAILED)
    return false;

  mapped->data += mapped->window_offset;
  mapped->fp = fp;
  mapped->writable = writable;

  return true;
}

static void
squash_mapped_file_destroy (SquashMappedFile* mapped) {
  if (mapped->data != MAP_FAILED) {
    munmap (mapped->data - mapped->window_offset, mapped->length + mapped->window_offset);
    if (mapped->writable) {
      fseeko (mapped->fp, mapped->length, SEEK_CUR);
      ftruncate (fileno (mapped->fp), ftello (mapped->fp));
    }
  }
}
/**
 * @endcond INTERNAL
 */

static SquashStatus
squash_splice_stream (SquashStream* stream, FILE* input, FILE* output, size_t length) {
  assert (stream != NULL);
  assert (input != NULL);
  assert (output != NULL);

  SquashStatus res = SQUASH_OK;
  uint8_t* input_buf = malloc (SQUASH_FILE_BUF_SIZE);
  uint8_t* output_buf = malloc (SQUASH_FILE_BUF_SIZE);

  const SquashStreamType stream_type = stream->stream_type;

  while (length == 0 ||
         (stream_type == SQUASH_STREAM_COMPRESS && stream->total_in < length) ||
         (stream_type == SQUASH_STREAM_DECOMPRESS && stream->total_out < length)) {
    const size_t remaining = length - stream->total_in;

    if (length == 0 || stream_type == SQUASH_STREAM_DECOMPRESS) {
      stream->avail_in = SQUASH_FILE_BUF_SIZE;
    } else {
      stream->avail_in = (remaining < SQUASH_FILE_BUF_SIZE) ? remaining : SQUASH_FILE_BUF_SIZE;
    }

    {
      assert (stream->avail_in != 0);
      const size_t bytes_read = fread (input_buf, 1, stream->avail_in, input);
      if (bytes_read == 0) {
        if (stream_type == SQUASH_STREAM_COMPRESS) {
          if (feof (input)) {
            length = stream->total_in + bytes_read;
          } else {
            res = squash_error (SQUASH_IO);
            break;
          }
        }
      }
      stream->avail_in = bytes_read;
      stream->next_in = input_buf;
    }

    do {
      if (stream_type == SQUASH_STREAM_COMPRESS) {
        stream->avail_out = SQUASH_FILE_BUF_SIZE;
      } else {
        if (length == 0) {
          stream->avail_out = SQUASH_FILE_BUF_SIZE;
        } else {
          stream->avail_out = (remaining < SQUASH_FILE_BUF_SIZE) ? remaining : SQUASH_FILE_BUF_SIZE;
        }
      }
      stream->next_out = output_buf;
      const size_t current_output_size = stream->avail_out;

      if (stream->avail_in == 0 || (length != 0 && stream_type == SQUASH_STREAM_COMPRESS && (stream->avail_in + stream->total_in) == length)) {
        res = squash_stream_finish (stream);
        if (res == SQUASH_OK)
          length = stream->total_out;
      } else {
        res = squash_stream_process (stream);
      }

      if (res > 0 && stream->avail_out > 0 && stream->avail_out != SQUASH_FILE_BUF_SIZE) {
        const size_t bytes_to_write = current_output_size - stream->avail_out;
        const size_t bytes_written = fwrite (output_buf, 1, bytes_to_write, output);
        if (bytes_written != bytes_to_write) {
          res = squash_error (SQUASH_IO);
        }
      }

      stream->avail_out = 0;
      stream->next_out = NULL;
    } while (res == SQUASH_PROCESSING);

    if (res < 0)
      break;
  }

  free (input_buf);
  free (output_buf);

  return res;
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
squash_splice_codec_with_options (FILE* fp_in,
                                  FILE* fp_out,
                                  size_t length,
                                  SquashStreamType stream_type,
                                  SquashCodec* codec,
                                  SquashOptions* options) {
  SquashStatus res = SQUASH_FAILED;

  assert (fp_in != NULL);
  assert (fp_out != NULL);
  assert (stream_type == SQUASH_STREAM_COMPRESS || stream_type == SQUASH_STREAM_DECOMPRESS);
  assert (codec != NULL);

  if (codec->impl.create_stream == NULL) {
    SquashMappedFile mapped_in = { MAP_FAILED, 0, };
    SquashMappedFile mapped_out = { MAP_FAILED, 0, };

    bool success = squash_mapped_file_init (&mapped_in, fp_in, (stream_type == SQUASH_STREAM_COMPRESS) ? length : 0, false);
    if (!success)
      goto nomap;

    if (stream_type == SQUASH_STREAM_COMPRESS) {
      const size_t output_length = squash_codec_get_max_compressed_size(codec, mapped_in.length) + 4096;

      success = squash_mapped_file_init (&mapped_out, fp_out, output_length, true);
      if (success) {
        size_t compressed_length = output_length;
        res = squash_codec_compress_with_options (codec, &compressed_length, mapped_out.data, mapped_in.length, mapped_in.data, options);
        if (res == SQUASH_OK) {
          mapped_out.length = compressed_length;
          squash_mapped_file_destroy (&mapped_in);
          squash_mapped_file_destroy (&mapped_out);
        }
        return res;
      } else {
        uint8_t* compressed = malloc (output_length);
        if (compressed != NULL) {
          size_t compressed_length = output_length;
          res = squash_codec_compress_with_options (codec, &compressed_length, compressed, mapped_in.length, mapped_in.data, options);
          if (res == SQUASH_OK) {
            const size_t written = fwrite (compressed, 1, compressed_length, fp_out);
            if (written != compressed_length)
              res = squash_error (SQUASH_IO);
          }
          free (compressed);
        } else {
          res = squash_error (SQUASH_MEMORY);
        }

        squash_mapped_file_destroy (&mapped_in);
        return res;
      }
    } else { /* stream_type == SQUASH_STREAM_DECOMPRESS */
      size_t output_length;
      const bool knows_uncompressed =
        (squash_codec_get_info (codec) & SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) == SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE;

      if (knows_uncompressed) {
        output_length = squash_codec_get_uncompressed_size (codec, mapped_in.length, mapped_in.data);
        if (output_length == 0) {
          squash_mapped_file_destroy (&mapped_in);
          return squash_error (SQUASH_INVALID_BUFFER);
        }
      } else {
        output_length = squash_npot (mapped_in.length) << 3;
      }

      do {
        success = squash_mapped_file_init (&mapped_out, fp_out, output_length, true);
        if (success) {
          size_t olen = output_length;
          res = squash_codec_decompress_with_options (codec, &olen, mapped_out.data, mapped_in.length, mapped_in.data, options);
          assert (res == SQUASH_OK);
          if (res == SQUASH_OK) {
            mapped_out.length = olen;
            squash_mapped_file_destroy (&mapped_out);
            break;
          }
        } else {
          size_t decompressed_length = output_length;
          uint8_t* decompressed = malloc (decompressed_length);
          if (decompressed == NULL) {
            squash_mapped_file_destroy (&mapped_in);
            goto nomap;
          }
          res = squash_codec_decompress_with_options (codec, &decompressed_length, decompressed, mapped_in.length, mapped_in.data, options);
          if (res == SQUASH_OK) {
            size_t bytes_written = fwrite (decompressed, 1, decompressed_length, fp_out);
            if (bytes_written != decompressed_length)
              res = squash_error (SQUASH_IO);
          } else {
            output_length <<= 1;
          }
          free (decompressed);
        }
      } while (res == SQUASH_BUFFER_FULL && !knows_uncompressed);
    }

    squash_mapped_file_destroy (&mapped_in);

    return res;
  }

 nomap:

  res = SQUASH_OK;

  SquashStream* stream = NULL;

  stream = squash_codec_create_stream_with_options (codec, stream_type, options);
  if (stream == NULL) {
    res = squash_error (SQUASH_FAILED);
    goto nomap_cleanup;
  }

  res = squash_splice_stream (stream, fp_in, fp_out, length);

 nomap_cleanup:

  squash_object_unref (stream);

  return res;
}

/**
 * @brief Close a file
 *
 * If @a file is a compressor the stream will finish compressing,
 * writing any buffered data.  For codecs which do not provide a
 * native streaming interface, *all* of the actual compression will
 * take place during this call.  In other words, it may block for a
 * non-trivial period.  If this is a problem please file a bug against
 * Squash (including your use case), and we can discuss adding a
 * function call which will simply abort compression.
 *
 * In addition to freeing the @ref SquashFile instance, this function
 * will close the underlying *FILE* pointer.  If you wish to continue
 * using the *FILE* for something else, use @ref squash_file_free
 * instead.
 *
 * @param file file to close
 * @return @ref SQUASH_OK on success or a negative error code on
 *   failure
 */
SquashStatus
squash_file_close (SquashFile* file) {
  FILE* fp = NULL;
  SquashStatus res = squash_file_free (file, &fp);
  if (fp != NULL)
    fclose (fp);
  return res;
}

/**
 * @brief Free a file
 *
 * This function will free the @ref SquashFile, but unlike @ref
 * squash_file_close it will not actually close the underlying *FILE*

 * pointer.  Instead, it will return the value in the @a fp argument,
 * allowing you to further manipulate it.
 *
 * @param file file to free
 * @param[out] fp location to store the underlying *FILE* pointer
 * @return @ref SQUASH_OK on success or a negative error code on
 *   failure
 */
SquashStatus
squash_file_free (SquashFile* file, FILE** fp) {
  SquashStatus res = SQUASH_OK;

  assert (file != NULL);
  assert (fp != NULL);

  if (file->stream != NULL && file->stream->stream_type == SQUASH_STREAM_COMPRESS)
    res = squash_file_write_internal (file, 0, NULL, SQUASH_OPERATION_FINISH);

  *fp = file->fp;
  squash_object_unref (file->stream);
  squash_object_unref (file->options);
  free (file);

  return res;
}

/**
 * @}
 */
