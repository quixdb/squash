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

#if !defined(_WIN32)
#include <unistd.h>
#if !defined(_POSIX_MAPPED_FILES)
#error No mmap implementation
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif

#include <limits.h>
#include <assert.h>

#include "internal.h"

/**
 * @brief Remap a mapped file
 *
 * @param mapped The mapped file to remap
 * @param offset The new offset to map
 * @param length The new length to map, or 0 for the entire file
 * @return true on success, false on failure
 */
bool
squash_mapped_file_remap (SquashMappedFile* mapped, size_t offset, size_t length) {
  long old_pos;

  assert (mapped != NULL);

  int fd = fileno (mapped->fp);
  if (fd == -1)
    return false;

  if (length == 0) {
    struct stat sb = { 0, };

    if (fstat (fd, &sb) == -1)
      return false;

    length = sb.st_size - offset;
  }

  old_pos = ftell (mapped->fp);
  if (old_pos == -1)
    return false;

  #if LONG_MAX < SIZE_MAX
  if ((offset + length) > LONG_MAX)
    return false;
  #endif

  old_pos = ftell (mapped->fp);

  if (ftruncate (fd, (off_t) (offset + length)) == -1)
    return false;

  if (fseek (mapped->fp, (long) (offset + length), SEEK_SET) == -1)
    return false;

  if (mapped->data != NULL) {
    munmap (mapped->data, mapped->data_length);
    mapped->data = NULL;
  }

  mapped->data = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) offset);
  if (mapped->data == MAP_FAILED)
    goto handle_failure;

  mapped->data_length = length;

  return true;

 handle_failure:
  if (mapped->data == MAP_FAILED)
    mapped->data = NULL;

  fseek (mapped->fp, old_pos, SEEK_SET);
  ftruncate (fd, (off_t) old_pos);

  return false;
}

SquashMappedFile*
squash_mapped_file_new_full (FILE* fp, bool close_fp, size_t offset, size_t length) {
  SquashMappedFile* mapped = malloc (sizeof (SquashMappedFile));
  mapped->data = NULL;
  mapped->data_length = 0;
  mapped->data_offset = 0;

  mapped->fp = fp;
  mapped->close_fp = close_fp;

  if (!squash_mapped_file_remap (mapped, offset, length)) {
    free (mapped);
    mapped = NULL;
  }

  return mapped;
}

void
squash_mapped_file_free (SquashMappedFile* mapped) {
  if (mapped == NULL)
    return;


  if (mapped->data != NULL) {
    munmap (mapped->data, mapped->data_length);
    mapped->data = NULL;
  }

  if (mapped->close_fp)
    fclose (mapped->fp);

  free (mapped);
}

SquashMappedFile*
squash_mapped_file_new (FILE* fp, bool close_fp) {
  return squash_mapped_file_new_full (fp, close_fp, 0, 0);
}
