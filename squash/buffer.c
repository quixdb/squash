/* Copyright (c) 2013-2015 The Squash Authors
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

#include <assert.h>
#include <squash/internal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t
squash_buffer_npot_page (size_t value) {
  const size_t page_size = squash_get_page_size ();

  if (value < page_size)
    return page_size;
  else
    return squash_npot (value);
}

static bool
squash_buffer_ensure_allocation (SquashBuffer* buffer, size_t allocation) {
  assert (buffer != NULL);

  if (allocation > buffer->allocated) {
    const size_t next_allocation = squash_buffer_npot_page (allocation);
    /* To catch very very large requests */
    if (SQUASH_LIKELY(next_allocation > allocation))
      allocation = next_allocation;
    uint8_t* mem = (uint8_t*) realloc (buffer->data, allocation);
    if (mem == NULL)
      return false;
    buffer->allocated = allocation;
    buffer->data = mem;
  }

  return true;
}

/**
 * @brief Create a new buffer
 * @private
 *
 * @param preallocated_len Size of data (in bytes) to pre-allocate
 * @return A new buffer, or *NULL* if the requested size could not be
 *   allocated
 */
SquashBuffer*
squash_buffer_new (size_t preallocated_len) {
  SquashBuffer* buffer = (SquashBuffer*) malloc (sizeof (SquashBuffer));
  if (SQUASH_UNLIKELY(buffer == NULL))
    return NULL;

  buffer->data = NULL;
  buffer->size = 0;
  buffer->allocated = 0;
  const bool allocated = squash_buffer_ensure_allocation (buffer, preallocated_len);
  if (SQUASH_UNLIKELY(!allocated))
    return (free (buffer), NULL);

  return buffer;
}

bool
squash_buffer_set_size (SquashBuffer* buffer, size_t size) {
  assert (buffer != NULL);

  if (size > buffer->allocated)
    if (!squash_buffer_ensure_allocation (buffer, size))
      return false;

  buffer->size = size;

  return true;
}

void
squash_buffer_clear (SquashBuffer* buffer) {
  assert (buffer != NULL);

  free (buffer->data);
  buffer->data = NULL;
  buffer->allocated = 0;
  buffer->size = 0;
}

bool
squash_buffer_append (SquashBuffer* buffer, size_t data_size, const uint8_t data[SQUASH_ARRAY_PARAM(data_size)]) {
  assert (buffer != NULL);

  if (data_size == 0)
    return true;

  const size_t start_pos = buffer->size;

  if (!squash_buffer_set_size (buffer, buffer->size + data_size))
    return false;

  memcpy (buffer->data + start_pos, data, data_size);

  return true;
}

bool
squash_buffer_append_c (SquashBuffer* buffer, char c) {
  return squash_buffer_append (buffer, 1, (uint8_t*) &c);
}

void
squash_buffer_free (SquashBuffer* buffer) {
  if (buffer == NULL)
    return;

  if (buffer->data != NULL) {
    free (buffer->data);
  }
  free (buffer);
}

uint8_t*
squash_buffer_release (SquashBuffer* buffer,
                       size_t* size) {
  uint8_t* data = buffer->data;
  if (size != NULL)
    *size = buffer->size;

  free (buffer);

  return data;
}

void
squash_buffer_steal (SquashBuffer* buffer,
                     size_t data_size,
                     size_t data_allocated,
                     uint8_t data[SQUASH_ARRAY_PARAM(data_allocated)]) {
  squash_buffer_clear (buffer);

  buffer->data = data;
  buffer->allocated = data_allocated;
  buffer->size = data_size;
}
