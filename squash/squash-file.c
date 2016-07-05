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

#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200112L

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <assert.h>
#include "squash-internal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "squash/tinycthread/source/tinycthread.h"

/* #define SQUASH_MMAP_IO */

/**
 * @cond INTERNAL
 */

struct SquashFile_ {
  FILE* fp;
  mtx_t mtx;
  bool eof;
  SquashStream* stream;
  SquashStatus last_status;
  SquashCodec* codec;
  SquashOptions* options;
  uint8_t buf[SQUASH_FILE_BUF_SIZE];
#if defined(SQUASH_MMAP_IO)
  SquashMappedFile map;
#endif
};

/**
 * @endcond INTERNAL
 */

/**
 * @defgroup SquashFile SquashFile
 * @brief stdio-like API and utilities
 *
 * These functions provide an API which should be familiar for those
 * used to dealing with the standard I/O functions.
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
 * you should call @ref squash_options_parse yourself and pass the
 * results to @ref squash_file_open_with_options (which will only fail
 * due to the underlying *fopen* failing).
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open_with_options
 */
SquashFile*
squash_file_open (SquashCodec* codec, const char* filename, const char* mode, ...) {
  va_list ap;
  SquashOptions* options;

  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  va_start (ap, mode);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_file_open_with_options (codec, filename, mode, options);
}

/**
 * @brief Open a file using a with the specified options
 *
 * @a filename is assumed to be UTF-8 encoded.  On Windows, this
 * function will call @ref squash_file_wopen_with_options internally.
 * On other platforms, filenames on disk are assumed to be in UTF-8
 * format, therefore the @a filename is passed through to `fopen`
 * without any conversion.
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_open
 * @see squash_file_wopen_with_options
 */
SquashFile*
squash_file_open_with_options (SquashCodec* codec, const char* filename, const char* mode, SquashOptions* options) {
  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

#if !defined(_WIN32)
  FILE* fp = fopen (filename, mode);
  if (HEDLEY_LIKELY(fp == NULL))
    return NULL;

  return squash_file_steal_with_options (codec, fp, options);
#else
  wchar_t* wfilename = squash_charset_utf8_to_wide (filename);
  if (wfilename == NULL)
    return NULL;

  wchar_t* wmode = squash_charset_utf8_to_wide (mode);
  if (wmode == NULL) {
    squash_free (wfilename);
    return NULL;
  }

  SquashFile* file = squash_file_wopen_with_options (codec, wfilename, wmode, options);
  squash_free (wfilename);
  squash_free (wmode);
  return file;
#endif
}

/**
 * @brief Open a file using a with the specified options
 *
 * On non-Windows platforms this function will convert the @a filename
 * to UTF-8 and call @ref squash_file_open_with_options.  On Windows,
 * it will use @a _wfopen.
 *
 * @param filename name of the file to open
 * @param mode file mode
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_wopen
 * @see squash_file_open
 */
#if defined(SQUASH_ENABLE_WIDE_CHAR_API) || defined(_WIN32)
#if !defined(SQUASH_ENABLE_WIDE_CHAR_API)
static
#endif
SquashFile*
squash_file_wopen_with_options (SquashCodec* codec, const wchar_t* filename, const wchar_t* mode, SquashOptions* options) {
  assert (codec != NULL);
  assert (filename != NULL);
  assert (mode != NULL);

#if !defined(_WIN32)
  char* nfilename = squash_charset_wide_to_utf8 (filename);
  if (nfilename == NULL)
    return NULL;

  char* nmode = squash_charset_wide_to_utf8 (mode);
  if (nmode == NULL) {
    squash_free (nfilename);
    return NULL;
  }

  SquashFile* file = squash_file_open_with_options (codec, nfilename, nmode, options);
  squash_free (nfilename);
  squash_free (nmode);
  return file;
#else
  FILE* fp = _wfopen (filename, mode);
  if (HEDLEY_UNLIKELY(fp == NULL))
    return NULL;

  return squash_file_steal_with_options (codec, fp, options);
#endif
}
#endif /* defined(SQUASH_ENABLE_WIDE_CHAR_API) || defined(_WIN32) */

/**
 * @brief Open an existing stdio file
 *
 * Note that Squash expects to have exclusive access to @a fp.  When
 * possible, Squash will acquire @a fp's lock (using flockfile) in
 * this function and will not release it until the @ref SquashFile
 * instance is destroyed.
 *
 * @warning On Windows you should not use this function unless the
 * code which opened the file descriptor is using the same runtime;
 * see http://siomsystems.com/mixing-visual-studio-versions/ for more
 * information.
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param ... options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal_with_options
 */
SquashFile*
squash_file_steal (SquashCodec* codec, FILE* fp, ...) {
  va_list ap;
  SquashOptions* options;

  assert (fp != NULL);
  assert (codec != NULL);

  va_start (ap, fp);
  options = squash_options_newv (codec, ap);
  va_end (ap);

  return squash_file_steal_with_options (codec, fp, options);
}

/**
 * @brief Open an existing stdio file with the specified options
 *
 * @warning On Windows you should not use this function unless the
 * code which opened the file descriptor is using the same runtime;
 * see http://siomsystems.com/mixing-visual-studio-versions/ for more
 * information.
 *
 * @param fp the stdio file to use
 * @param codec codec to use
 * @param options options
 * @return The opened file, or *NULL* on error
 * @see squash_file_steal
 */
SquashFile*
squash_file_steal_with_options (SquashCodec* codec, FILE* fp, SquashOptions* options) {
  assert (fp != NULL);
  assert (codec != NULL);

  SquashFile* file = squash_malloc (sizeof (SquashFile));
  if (file == NULL)
    return NULL;

  file->fp = fp;
  file->eof = false;
  file->stream = NULL;
  file->last_status = SQUASH_OK;
  file->codec = codec;
  file->options = (options != NULL) ? squash_object_ref (options) : NULL;
#if defined(SQUASH_MMAP_IO)
  file->map = squash_mapped_file_empty;
#endif

  mtx_init (&(file->mtx), mtx_recursive);

  SQUASH_FLOCKFILE(fp);

  return file;
}

/**
 * @brief Read from a compressed file
 *
 * Attempt to read @a decompressed_size bytes of decompressed data
 * into the @a decompressed buffer.  The number of bytes of compressed
 * data read from the input file may be **significantly** more, or less,
 * than @a decompressed_size.
 *
 * The number of decompressed bytes successfully read from the file
 * will be stored in @a decompressed_read after this function is
 * execute.  This value will never be greater than @a
 * decompressed_size, but it may be less if there was an error or
 * the end of the input file was reached.
 *
 * @note Squash can, and frequently will, read more data from the
 * input file than necessary to produce the requested amount of
 * decompressed data.  There is no way to know how much input will be
 * required to produce the reuested output, or even how much input
 * *was* used.
 *
 * @param file the file to read from
 * @param decompressed_size number of bytes to attempt to write to @a decompressed
 * @param decompressed buffer to write the decompressed data to
 * @return the result of the operation
 * @retval SQUASH_OK successfully read some data
 * @retval SQUASH_END_OF_STREAM the end of the file was reached
 */
SquashStatus
squash_file_read (SquashFile* file,
                  size_t* decompressed_size,
                  uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)]) {
  assert (file != NULL);
  assert (decompressed_size != NULL);
  assert (decompressed != NULL);

  squash_file_lock (file);
  SquashStatus res = squash_file_read_unlocked (file, decompressed_size, decompressed);
  squash_file_unlock (file);

  return res;
}

/**
 * @brief Read from a compressed file
 *
 * This function is the same as @ref squash_file_read, except it will
 * not acquire a lock on the @ref SquashFile instance.  It should be
 * used only when there is no possibility of other threads accessing
 * the file, or if you have already acquired the lock with @ref
 * squash_file_lock.
 *
 * @param file the file to read from
 * @param decompressed_size number of bytes to attempt to write to @a decompressed
 * @param decompressed buffer to write the decompressed data to
 * @return the result of the operation
 * @retval SQUASH_OK successfully read some data
 * @retval SQUASH_END_OF_STREAM the end of the file was reached
 */
SquashStatus
squash_file_read_unlocked (SquashFile* file,
                           size_t* decompressed_size,
                           uint8_t decompressed[HEDLEY_ARRAY_PARAM(*decompressed_size)]) {
  assert (file != NULL);
  assert (decompressed_size != NULL);
  assert (decompressed != NULL);

  if (HEDLEY_UNLIKELY(file->last_status < 0))
    return file->last_status;

  if (file->stream == NULL) {
    file->stream = squash_codec_create_stream_with_options (file->codec, SQUASH_STREAM_DECOMPRESS, file->options);
    if (HEDLEY_UNLIKELY(file->stream == NULL)) {
      return file->last_status = squash_error (SQUASH_FAILED);
    }
  }
  SquashStream* stream = file->stream;

  assert (stream->next_out == NULL);
  assert (stream->avail_out == 0);

  if (stream->state == SQUASH_STREAM_STATE_FINISHED) {
    *decompressed_size = 0;
    return SQUASH_END_OF_STREAM;
  }

  file->stream->next_out = decompressed;
  file->stream->avail_out = *decompressed_size;

  while (stream->avail_out != 0) {
    if (file->last_status < 0) {
      break;
    }

    if (stream->state == SQUASH_STREAM_STATE_FINISHED)
      break;

    if (file->last_status == SQUASH_PROCESSING) {
      if (stream->state < SQUASH_STREAM_STATE_FINISHING) {
        file->last_status = squash_stream_process (stream);
      } else {
        file->last_status = squash_stream_finish (stream);
      }

      continue;
    }

    assert (file->last_status == SQUASH_OK);

#if defined(SQUASH_MMAP_IO)
    if (file->map.data != MAP_FAILED)
      squash_mapped_file_destroy (&(file->map), true);

    if (squash_mapped_file_init_full(&(file->map), file->fp, SQUASH_FILE_BUF_SIZE, true, false)) {
      stream->next_in = file->map.data;
      stream->avail_in = file->map.size;
    } else
#endif
    {
      stream->next_in = file->buf;
      stream->avail_in = SQUASH_FREAD_UNLOCKED(file->buf, 1, SQUASH_FILE_BUF_SIZE, file->fp);
    }

    if (stream->avail_in == 0) {
      if (feof (file->fp)) {
        file->last_status = squash_stream_finish (stream);
      } else {
        file->last_status = squash_error (SQUASH_IO);
        break;
      }
    } else {
      file->last_status = squash_stream_process (stream);
    }
  }

  *decompressed_size = (stream->next_out - decompressed);

  stream->next_out = 0;
  stream->avail_out = 0;

  return file->last_status;
}

static SquashStatus
squash_file_write_internal (SquashFile* file,
                            size_t uncompressed_size,
                            const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)],
                            SquashOperation operation) {
  SquashStatus res;

  assert (file != NULL);

  if (HEDLEY_UNLIKELY(file->last_status < 0))
    return file->last_status;

  if (file->stream == NULL) {
    file->stream = squash_codec_create_stream_with_options (file->codec, SQUASH_STREAM_COMPRESS, file->options);
    if (HEDLEY_UNLIKELY(file->stream == NULL)) {
      res = squash_error (SQUASH_FAILED);
      goto cleanup;
    }
  }

  assert (file->stream->next_in == NULL);
  assert (file->stream->avail_in == 0);
  assert (file->stream->next_out == NULL);
  assert (file->stream->avail_out == 0);

  file->stream->next_in = uncompressed;
  file->stream->avail_in = uncompressed_size;

  file->stream->next_in = uncompressed;
  file->stream->avail_in = uncompressed_size;

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
      size_t bytes_written = SQUASH_FWRITE_UNLOCKED(file->buf, 1, SQUASH_FILE_BUF_SIZE - file->stream->avail_out, file->fp);
      if (bytes_written != SQUASH_FILE_BUF_SIZE - file->stream->avail_out) {
        res = SQUASH_IO;
        goto cleanup;
      }
    }
  } while (res == SQUASH_PROCESSING);

 cleanup:

  if (file->stream != NULL) {
    file->stream->next_in = NULL;
    file->stream->avail_in = 0;
    file->stream->next_out = NULL;
    file->stream->avail_out = 0;
  }

  return file->last_status = res;
}

/**
 * @brief Write data to a compressed file
 *
 * Attempt to write the compressed equivalent of @a uncompressed to a
 * file.  The number of bytes of compressed data written to the output
 * file may be **significantly** more, or less, than the @a
 * uncompressed_size.
 *
 * @note It is likely the compressed data will actually be buffered,
 * not immediately written to the file.  For codecs which support
 * flushing you can use @ref squash_file_flush to flush the data,
 * otherwise the data may not be written until @ref squash_file_close
 * or @ref squash_file_free is called.
 *
 * @param file file to write to
 * @param uncompressed_size number of bytes of uncompressed data in
 *   @a uncompressed to attempt to write
 * @param uncompressed data to write
 * @return result of the operation
 */
SquashStatus
squash_file_write (SquashFile* file,
                   size_t uncompressed_size,
                   const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)]) {
  squash_file_lock (file);
  SquashStatus res = squash_file_write_unlocked (file, uncompressed_size, uncompressed);
  squash_file_unlock (file);

  return res;
}

#if !defined(HAVE__VSCWPRINTF)
static FILE* squash_dev_null = NULL;
static once_flag squash_dev_null_once = ONCE_FLAG_INIT;

#if defined(__GNUC__)
__attribute__((__destructor__))
static void
squash_dev_null_destroy (void) {
  if (squash_dev_null != NULL)
    fclose (squash_dev_null);
}
#endif

static void
squash_dev_null_init (void) {
  squash_dev_null = fopen ("/dev/null", "wb+");
}

/* This is one of the ugliest hacks I've ever done.  It's not my idea
 * (I stole it from
 * https://stackoverflow.com/questions/4107947/how-to-determine-buffer-size-for-vswprintf-under-linux-gcc).
 *
 * C99/C11 define vswprintf as returning -1 on error, unlike vsnprintf
 * which returns "… the number of characters that would have been
 * written had n been sufficiently large, not counting the terminating
 * null character, or a negative value if a runtime-constraint
 * violation occurred."
 *
 * Windows' vsnprintf implementation doesn't actually conform to C99;
 * it will return -1.  However, they have a family of functions to
 * calculate the length which would be required which includes
 * wide-character versions.  Specifically, _vscwprintf.
 *
 * In order to make up for the deficiencies of the standard, we open a
 * FILE* and use vfwprintf to find the length required.  It's slow and
 * ugly, but it works. */
static int
_vscwprintf (const wchar_t *format, va_list ap) {
  va_list apc;
  int r;

  call_once (&squash_dev_null_once, squash_dev_null_init);
  if (squash_dev_null == NULL)
    return -1;

  va_copy (apc, ap);
  r = vfwprintf (squash_dev_null, format, apc);
  va_end (apc);

  fclose (squash_dev_null);

  return r;
}
#endif

#if !defined(SQUASH_ENABLE_WIDE_CHAR_API)
static
#endif
SquashStatus
squash_file_vwprintf (SquashFile* file,
                      const wchar_t* format,
                      va_list ap) {
  SquashStatus res = SQUASH_OK;
  int size;
  wchar_t* buf = NULL;

  assert (file != NULL);
  assert (format != NULL);

  size = _vscwprintf (format, ap);
  if (HEDLEY_UNLIKELY(size < 0))
    return squash_error (SQUASH_FAILED);

  buf = calloc (size + 1, sizeof (wchar_t));
  if (HEDLEY_UNLIKELY(buf == NULL))
    return squash_error (SQUASH_MEMORY);

#if !defined(_WIN32)
  const int written = vswprintf (buf, size + 1, format, ap);
#else
  const int written = _vsnwprintf (buf, size + 1, format, ap);
#endif
  if (HEDLEY_UNLIKELY(written != size)) {
    res = squash_error (SQUASH_FAILED);
  } else {
    size_t data_size;
    char* data;
    bool conv_success =
      squash_charset_convert (&data_size, &data, "UTF-8",
                              size * sizeof(wchar_t), (char*) buf, squash_charset_get_wide ());

    if (HEDLEY_LIKELY(conv_success))
      res = squash_file_write (file, data_size, (uint8_t*) data);
    else
      res = squash_error (SQUASH_FAILED);
  }

  squash_free (buf);

  return res;
}

#if defined(__MINGW32__) || defined(__MINGW64__) || defined(__GNUC__)
#  pragma GCC diagnostic push
#  if defined(CFLAG_Wsuggest_attribute_format)
#    pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#  endif
#  if defined(CFLAG_Wmissing_format_attribute)
#    pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#  endif
#  if defined(CFLAG_Wformat_nonliteral)
#    pragma GCC diagnostic ignored "-Wformat-nonliteral"
#  endif
#endif

SquashStatus
squash_file_vprintf (SquashFile* file,
                     const char* format,
                     va_list ap) {
  SquashStatus res = SQUASH_OK;
  int size;
  char* heap_buf = NULL;

  assert (file != NULL);
  assert (format != NULL);

#if defined(_WIN32)
  size = _vscprintf (format, ap);
  if (HEDLEY_UNLIKELY(size < 0))
    return squash_error (SQUASH_FAILED);
#else
  char buf[256];
  size = vsnprintf (buf, sizeof (buf), format, ap);
  if (HEDLEY_UNLIKELY(size < 0))
    return squash_error (SQUASH_FAILED);
  else if (size >= (int) sizeof (buf))
#endif
  {
    heap_buf = squash_malloc (size + 1);
    if (HEDLEY_UNLIKELY(heap_buf == NULL))
      return squash_error (SQUASH_MEMORY);

    const int written = vsnprintf (heap_buf, size + 1, format, ap);
    if (HEDLEY_UNLIKELY(written != size))
      res = squash_error (SQUASH_FAILED);
  }

  if (HEDLEY_LIKELY(res == SQUASH_OK)) {
    res = squash_file_write (file, size,
#if !defined(_WIN32)
                             (heap_buf == NULL) ? (uint8_t*) buf :
#endif
                             (uint8_t*) heap_buf);
  }

  squash_free (heap_buf);

  return res;
}

SquashStatus
squash_file_printf (SquashFile* file,
                    const char* format,
                    ...) {
  SquashStatus res;
  va_list ap;

  va_start (ap, format);
  res = squash_file_vprintf (file, format, ap);
  va_end (ap);

  return res;
}

#if defined(__MINGW32__) || defined(__MINGW64__)
#pragma GCC diagnostic pop
#endif

/**
 * @brief Write data to a compressed file without acquiring the lock
 *
 * This function is the same as @ref squash_file_write, except it will
 * not acquire a lock on the @ref SquashFile instance.  It should be
 * used only when there is no possibility of other threads accessing
 * the file, or if you have already acquired the lock with @ref
 * squash_file_lock.
 *
 * @param file file to write to
 * @param uncompressed_size number of bytes of uncompressed data in
 *   @a uncompressed to attempt to write
 * @param uncompressed data to write
 * @return result of the operation
 */
SquashStatus
squash_file_write_unlocked (SquashFile* file,
                            size_t uncompressed_size,
                            const uint8_t uncompressed[HEDLEY_ARRAY_PARAM(uncompressed_size)]) {
  return squash_file_write_internal (file, uncompressed_size, uncompressed, SQUASH_OPERATION_PROCESS);
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
  assert (file != NULL);

  squash_file_lock (file);
  SquashStatus res = squash_file_flush_unlocked (file);
  squash_file_unlock (file);

  return res;
}

/**
 * @brief immediately write any buffered data to a file without
 * acquiring the lock
 * @brief Write data to a compressed file without acquiring the lock
 *
 * This function is the same as @ref squash_file_write, except it will
 * not acquire a lock on the @ref SquashFile instance.  It should be
 * used only when there is no possibility of other threads accessing
 * the file, or if you have already acquired the lock with @ref
 * squash_file_lock.
 *
 * @param file file to flush
 * @returns *TRUE* if flushing succeeeded, *FALSE* if flushing is not
 *   supported or there was another error.
 */
SquashStatus
squash_file_flush_unlocked (SquashFile* file) {
  SquashStatus res = squash_file_write_internal (file, 0, NULL, SQUASH_OPERATION_FLUSH);
  SQUASH_FFLUSH_UNLOCKED(file->fp);
  return res;
}

/**
 * @brief Determine whether the file has reached the end of file
 *
 * @param file file to check
 * @returns *TRUE* if EOF was reached, *FALSE* otherwise
 */
bool
squash_file_eof (SquashFile* file) {
  return (file->stream->state == SQUASH_STREAM_STATE_FINISHED) && (feof (file->fp));
}

/**
 * @brief Retrieve the last return value
 *
 * This will return the result of the previous function called on this
 * file.
 *
 * @param file file to examine
 * @return last status code returned by a function on @a file
 */
SquashStatus
squash_file_error (SquashFile* file) {
  return file->last_status;
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
  if (res > SQUASH_OK)
    res = SQUASH_OK;

  if (fp != NULL) {
    const SquashStatus cres = HEDLEY_LIKELY(fclose (fp) == 0) ? SQUASH_OK : squash_error (SQUASH_IO);
    if (HEDLEY_LIKELY(res > 0))
      res = cres;
  }
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
 * @warning On Windows you should not use this function unless Squash
 * is linked against the same runtime as the code you want to continue
 * using the file pointer from; see
 * http://siomsystems.com/mixing-visual-studio-versions/ for more
 * information.
 *
 * @param file file to free
 * @param[out] fp location to store the underlying *FILE* pointer
 * @return @ref SQUASH_OK on success or a negative error code on
 *   failure
 */
SquashStatus
squash_file_free (SquashFile* file, FILE** fp) {
  SquashStatus res = SQUASH_OK;

  if (HEDLEY_UNLIKELY(file == NULL)) {
    if (fp != NULL)
      *fp = NULL;
    return SQUASH_OK;
  }

  squash_file_lock (file);

  if (file->stream != NULL && file->stream->stream_type == SQUASH_STREAM_COMPRESS)
    res = squash_file_write_internal (file, 0, NULL, SQUASH_OPERATION_FINISH);

#if defined(SQUASH_MMAP_IO)
  squash_mapped_file_destroy (&(file->map), false);
#endif

  if (fp != NULL)
    *fp = file->fp;

  SQUASH_FUNLOCKFILE(file->fp);

  squash_object_unref (file->stream);
  squash_object_unref (file->options);

  squash_file_unlock (file);

  mtx_destroy (&(file->mtx));

  squash_free (file);

  return res;
}

/**
 * @brief Acquire the lock on a file
 *
 * @ref squash_file_read, @ref squash_file_write, and @ref
 * squash_file_flush are thread-safe.  This is accomplished by
 * acquiring a lock on while each function is operating in order to
 * ensure exclusive access.
 *
 * If, however, the programmer wishes to call a series of functions
 * and ensure that they are performed without interference, they can
 * manually acquire the lock with this function and use the unlocked
 * variants (@ref squash_file_read_unlocked, @ref
 * squash_file_write_unlocked, and @ref squash_file_flush_unlocked).
 *
 * @note This function has nothing to do with the kind of lock
 * acquired by the [flock](http://linux.die.net/man/2/flock) function.
 *
 * @param file the file to acquire the lock on
 */
void
squash_file_lock (SquashFile* file) {
  assert (file != NULL);

  mtx_lock (&(file->mtx));
}

/**
 * @brief Release the lock on a file
 *
 * This function releases the lock acquired by @ref squash_file_lock.
 * If you have not called that function *do not call this one*.
 *
 * @param file the file to release the lock on
 */
void
squash_file_unlock (SquashFile* file) {
  assert (file != NULL);

  mtx_unlock (&(file->mtx));
}

#if defined(SQUASH_ENABLE_WIDE_CHAR_API)
SquashFile*
squash_file_wopen (SquashCodec* codec, const wchar_t* filename, const wchar_t* mode, ...) {
  va_list ap;
  SquashOptions* options;

  assert (filename != NULL);
  assert (mode != NULL);
  assert (codec != NULL);

  va_start (ap, mode);
  options = squash_options_newvw (codec, ap);
  va_end (ap);

  return squash_file_wopen_with_options (codec, filename, mode, options);
}

SquashStatus
squash_file_wprintf (SquashFile* file,
                     const wchar_t* format,
                     ...) {
  va_list ap;

  va_start (ap, format);
  SquashStatus res = squash_file_vwprintf (file, format, ap);
  va_end (ap);

  return res;
}
#endif /* defined(SQUASH_ENABLE_WIDE_CHAR_API) */

/**
 * @}
 */
