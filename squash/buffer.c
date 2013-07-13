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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "config.h"
#include "squash.h"
#include "internal.h"

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

void
squash_buffer_set_size (SquashBuffer* buffer, size_t length) {
  if (length > buffer->allocated) {
    buffer->allocated = length - 1;
    buffer->allocated |= buffer->allocated >> 1;
    buffer->allocated |= buffer->allocated >> 2;
    buffer->allocated |= buffer->allocated >> 4;
    buffer->allocated |= buffer->allocated >> 8;
    buffer->allocated |= buffer->allocated >> 16;
    buffer->allocated++;

    buffer->data = (uint8_t*) realloc (buffer->data, buffer->allocated);
  }

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
  free (buffer->data);
  squash_buffer_free_container (buffer);
}

void
squash_buffer_free_container (SquashBuffer* buffer) {
  free (buffer);
}
