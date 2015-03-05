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

#ifndef SQUASH_MAPPED_FILE_H
#define SQUASH_MAPPED_FILE_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

typedef struct SquashMappedFile_s SquashMappedFile;

struct SquashMappedFile_s {
  void* data;
  size_t data_length;
  off_t data_offset;
  FILE* fp;
  bool close_fp;
};

bool              squash_mapped_file_remap    (SquashMappedFile* mapped, size_t offset, size_t length);
SquashMappedFile* squash_mapped_file_new_full (FILE* fp, bool close_fp, size_t offset, size_t length);
SquashMappedFile* squash_mapped_file_new      (FILE* fp, bool close_fp);
void              squash_mapped_file_free     (SquashMappedFile* mapped);

#endif /* SQUASH_MAPPED_FILE_H */
