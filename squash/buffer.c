/* Copyright (c) 2013 The Squash Authors
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

#include <squash/config.h>

#if defined(HAVE_MMAP)
#  define _POSIX_SOURCE
#  define _GNU_SOURCE
#  include <unistd.h>
#  if defined(HAVE_MREMAP)
#    include <sys/types.h>
#    include <sys/mman.h>
#endif
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "internal.h"

static size_t
squash_buffer_npot_page (size_t value) {
#if defined(_POSIX_SOURCE)
  static size_t page_size = 0;

  if (page_size == 0) {
    long ps = sysconf (_SC_PAGE_SIZE);
    page_size = (ps == -1) ? 8192 : ((size_t) ps);
  }

  if (value < page_size)
    value = page_size;
#else
  if (value < 4096)
    value = 4096;
#endif

  if ((value & (value - 1)) != 0) {
    value -= 1;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
  }

  return value;
}

static void
squash_buffer_ensure_allocation (SquashBuffer* buffer, size_t allocation) {
  if (allocation > buffer->allocated) {
    allocation = squash_buffer_npot_page (allocation);

#if defined(HAVE_MREMAP)
    if (buffer->data == NULL) {
      buffer->data = mmap (NULL, allocation, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      buffer->allocated = allocation;
      assert (buffer->data != MAP_FAILED);
    } else {
      buffer->data = mremap (buffer->data, buffer->allocated, allocation, MREMAP_MAYMOVE, NULL);
    }

    /* lseek (buffer->fd, (off_t) buffer->allocated - 1, SEEK_SET); */
    /* write (buffer->fd, "", 1); */
    /* buffer->data = mmap (NULL, buffer->allocated, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->fd, 0); */
    /* assert (buffer->data != MAP_FAILED); */
    /* fflush (stderr); */
#else
    buffer->data = (uint8_t*) realloc (buffer->data, buffer->allocated);
#endif
  }
}

/**
 * @brief Create a new buffer
 * @private
 *
 * @param preallocated_len Size of data (in bytes) to pre-allocate
 * @return A new buffer
 */
SquashBuffer*
squash_buffer_new (size_t preallocated_len) {
  SquashBuffer* buffer = (SquashBuffer*) malloc (sizeof (SquashBuffer));

#if defined(HAVE_MREMAP)
  buffer->data = NULL;
  buffer->length = 0;
  buffer->allocated = 0;

  if (preallocated_len > 0)
    squash_buffer_ensure_allocation (buffer, preallocated_len);
#else
  buffer->data = preallocated_len > 0 ? (uint8_t*) malloc (preallocated_len) : NULL;
  buffer->length = 0;
  buffer->allocated = preallocated_len;
#endif

  return buffer;
}

void
squash_buffer_set_size (SquashBuffer* buffer, size_t length) {
  if (length > buffer->allocated)
    squash_buffer_ensure_allocation (buffer, length);

  buffer->length = length;
}

void
squash_buffer_append (SquashBuffer* buffer, uint8_t* data, size_t data_length) {
  const size_t start_pos = buffer->length;

  squash_buffer_set_size (buffer, buffer->length + data_length);

  memcpy (buffer->data + start_pos, data, data_length);
}

void
squash_buffer_free (SquashBuffer* buffer) {
  if (buffer->data != NULL) {
#if defined(HAVE_MREMAP)
    munmap (buffer->data, buffer->allocated);
#else
    free (buffer->data);
#endif
  }
  free (buffer);
}
