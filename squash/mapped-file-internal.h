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
/* IWYU pragma: private, include <squash/internal.h> */

#ifndef SQUASH_FILE_INTERNAL_H
#define SQUASH_FILE_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#include <sys/mman.h>

SQUASH_BEGIN_DECLS

typedef struct SquashMappedFile_s {
  uint8_t* data;
  size_t size;
  size_t map_size;
  size_t window_offset;
  FILE* fp;
  bool writable;
} SquashMappedFile;

static const SquashMappedFile squash_mapped_file_empty = { MAP_FAILED, 0 };

SQUASH_NONNULL(1, 2) SQUASH_INTERNAL
bool squash_mapped_file_init_full (SquashMappedFile* mapped,
                                   FILE* fp,
                                   size_t size,
                                   bool size_is_suggestion,
                                   bool writable);
SQUASH_NONNULL(1, 2) SQUASH_INTERNAL
bool squash_mapped_file_init      (SquashMappedFile* mapped,
                                   FILE* fp,
                                   size_t size,
                                   bool writable);
SQUASH_NONNULL(1) SQUASH_INTERNAL
bool squash_mapped_file_destroy   (SquashMappedFile* mapped,
                                   bool success);

SQUASH_END_DECLS

#endif /* SQUASH_FILE_INTERNAL_H */
