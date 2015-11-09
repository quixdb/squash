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
/* IWYU pragma: private, include <squash/squash.h> */

#ifndef SQUASH_OBJECT_H
#define SQUASH_OBJECT_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <stdbool.h>

SQUASH_BEGIN_DECLS

SQUASH_API void*        squash_object_ref           (void* obj);
SQUASH_API void*        squash_object_unref         (void* obj);
SQUASH_API unsigned int squash_object_get_ref_count (void* obj);
SQUASH_API void*        squash_object_ref_sink      (void* obj);

typedef void (*SquashDestroyNotify) (void* data);

struct SquashObject_ {
  volatile unsigned int ref_count;
  volatile int is_floating;
  SquashDestroyNotify destroy_notify;
};

SQUASH_NONNULL(1)
SQUASH_API void         squash_object_init          (void* obj, bool is_floating, SquashDestroyNotify destroy_notify);
SQUASH_NONNULL(1)
SQUASH_API void         squash_object_destroy       (void* obj);

SQUASH_END_DECLS

#endif /* SQUASH_OBJECT_H */
