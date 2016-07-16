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

#include <assert.h>
#include "squash-internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

bool
squash_mapped_file_init_full (SquashMappedFile* mapped, FILE* fp, size_t size, bool size_is_suggestion, bool writable) {
  assert (mapped != NULL);
  assert (fp != NULL);

  if (mapped->data != MAP_FAILED)
    munmap (mapped->data - mapped->window_offset, mapped->size + mapped->window_offset);

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
    ires = ftruncate (fd, offset + (off_t) size);
    if (ires == -1)
      return false;
  } else {
    const size_t remaining = fp_stat.st_size - (size_t) offset;
    if (remaining > 0) {
      if (size == 0 || (size > remaining && size_is_suggestion)) {
        size = remaining;
      } else if (size > remaining) {
        return false;
      }
    } else {
      return false;
    }
  }
  mapped->size = size;

  int map_flags = MAP_SHARED;

#if defined(MAP_HUGETLB)
  size_t page_size = squash_get_huge_page_size ();
  if (page_size == 0) {
    page_size = squash_get_page_size ();
  } else {
    map_flags |= MAP_HUGETLB;
  }
#else
  const size_t page_size = squash_get_page_size ();
#endif
  mapped->window_offset = (size_t) offset % page_size;
  mapped->map_size = size + mapped->window_offset;

  if (writable)
    mapped->data = mmap (NULL, mapped->map_size, PROT_READ | PROT_WRITE, map_flags, fd, offset - mapped->window_offset);
  else
    mapped->data = mmap (NULL, mapped->map_size, PROT_READ, map_flags, fd, offset - mapped->window_offset);

  if (mapped->data == MAP_FAILED)
    return false;

  mapped->data += mapped->window_offset;
  mapped->fp = fp;
  mapped->writable = writable;

  return true;
}

bool
squash_mapped_file_init (SquashMappedFile* mapped, FILE* fp, size_t size, bool writable) {
  return squash_mapped_file_init_full (mapped, fp, size, false, writable);
}

bool
squash_mapped_file_destroy (SquashMappedFile* mapped, bool success) {
  if (mapped->data != MAP_FAILED) {
    munmap (mapped->data - mapped->window_offset, mapped->size + mapped->window_offset);
    mapped->data = MAP_FAILED;

    if (success) {
      const int sres = fseeko (mapped->fp, mapped->size, SEEK_CUR);
      if (HEDLEY_LIKELY(sres != -1)) {
        if (mapped->writable) {
          const off_t pos = ftello (mapped->fp);
          if (HEDLEY_LIKELY(pos != -1)) {
            const int tr = ftruncate (fileno (mapped->fp), pos);
            return HEDLEY_LIKELY(tr != -1) ? true : false;
          } else {
            return false;
          }
        }
      } else {
        return false;
      }
    }
  }

  return true;
}
