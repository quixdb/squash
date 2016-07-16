/* Copyright (c) 2013-2016 The Squash Authors
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
/* IWYU pragma: private, include "squash-internal.h" */

#ifndef SQUASH_BUFFER_INTERNAL_H
#define SQUASH_BUFFER_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

HEDLEY_BEGIN_C_DECLS

SQUASH_INTERNAL
SquashBuffer* squash_buffer_new            (size_t preallocated_len);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
bool          squash_buffer_append         (SquashBuffer* buffer, size_t data_size, const uint8_t data[HEDLEY_ARRAY_PARAM(data_size)]);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
bool          squash_buffer_append_c       (SquashBuffer* buffer, char c);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
void          squash_buffer_clear          (SquashBuffer* buffer);
SQUASH_INTERNAL
void          squash_buffer_free           (SquashBuffer* buffer);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
bool          squash_buffer_set_size       (SquashBuffer* buffer, size_t size);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
void          squash_buffer_steal          (SquashBuffer* buffer,
                                            size_t data_size,
                                            size_t data_allocated,
                                            uint8_t data[HEDLEY_ARRAY_PARAM(data_allocated)]);
HEDLEY_NON_NULL(1) SQUASH_INTERNAL
uint8_t*      squash_buffer_release        (SquashBuffer* buffer,
                                            size_t* size);

HEDLEY_END_C_DECLS

#endif /* SQUASH_SLIST_INTERNAL_H */
