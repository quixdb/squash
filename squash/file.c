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

#include "internal.h"

#ifndef SQUASH_FILE_BUF_SIZE
#  define SQUASH_FILE_BUF_SIZE ((size_t) (1024 * 1024))
#endif

struct _SquashFile {
  FILE* fp;
  bool eof;
  SquashStream* stream;
  SquashCodec* codec;
  SquashOptions* options;
  uint8_t buf[SQUASH_FILE_BUF_SIZE];
};

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
 * @param[out] decompressed_read number of bytes read
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
