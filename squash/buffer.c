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

#include <squash/config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "internal.h"

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
    allocation = squash_buffer_npot_page (allocation);
    buffer->allocated = allocation;
    uint8_t* mem = (uint8_t*) realloc (buffer->data, allocation);
    if (mem == NULL)
      return false;
    buffer->data = mem;
  }

  return true;
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

  buffer->data = preallocated_len > 0 ? (uint8_t*) malloc (preallocated_len) : NULL;
  buffer->length = 0;
  buffer->allocated = preallocated_len;

  return buffer;
}

bool
squash_buffer_set_size (SquashBuffer* buffer, size_t length) {
  assert (buffer != NULL);

  if (length > buffer->allocated)
    if (!squash_buffer_ensure_allocation (buffer, length))
      return false;

  buffer->length = length;

  return true;
}

void
squash_buffer_clear (SquashBuffer* buffer) {
  assert (buffer != NULL);

  free (buffer->data);
  buffer->data = NULL;
  buffer->allocated = 0;
  buffer->length = 0;
}

bool
squash_buffer_append (SquashBuffer* buffer, size_t data_length, uint8_t data[SQUASH_ARRAY_PARAM(data_length)]) {
  assert (buffer != NULL);

  if (data_length == 0)
    return true;

  const size_t start_pos = buffer->length;

  if (!squash_buffer_set_size (buffer, buffer->length + data_length))
    return false;

  memcpy (buffer->data + start_pos, data, data_length);

  return true;
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
