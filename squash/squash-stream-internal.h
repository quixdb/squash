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
/* IWYU pragma: private, include "squash-internal.h" */

#ifndef SQUASH_STREAM_INTERNAL_H
#define SQUASH_STREAM_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

HEDLEY_BEGIN_C_DECLS

struct SquashStreamPrivate_ {
  thrd_t thread;
  bool finished;

  mtx_t io_mtx;

  SquashOperation request;
  cnd_t request_cnd;

  SquashStatus result;
  cnd_t result_cnd;
};

#define SQUASH_OPERATION_INVALID ((SquashOperation) 0)
#define SQUASH_STATUS_INVALID ((SquashStatus) 0)

#define SQUASH_STREAM_ALLOC_SIZE SQUASH_ALLOC_SIZE(SquashStream)
#define SQUASH_STREAM_PRIVATE(stream) ((void*) (((uintptr_t) stream) + SQUASH_STREAM_ALLOC_SIZE))
#define SQUASH_STREAM_PRIVATE_C(T, stream) ((T*) SQUASH_STREAM_PRIVATE(stream))

HEDLEY_END_C_DECLS

#endif /* !defined(SQUASH_STREAM_INTERNAL_H) */
