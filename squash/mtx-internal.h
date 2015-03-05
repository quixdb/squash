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

#ifndef SQUASH_MTX_INTERNAL_H
#define SQUASH_MTX_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#include "tinycthread/source/tinycthread.h"

SQUASH_BEGIN_DECLS

#define SQUASH_MTX_NAME(name,suffix) _squash_ ## name ## _ ## suffix
#define SQUASH_MTX_DEFINE(name) \
  static once_flag SQUASH_MTX_NAME(name,init_flag) = ONCE_FLAG_INIT;            \
  static mtx_t SQUASH_MTX_NAME(name,mtx);                                       \
  static void SQUASH_MTX_NAME(name,init) (void);                                \
    static void SQUASH_MTX_NAME(name,init) (void) {                             \
    assert (mtx_init (&(SQUASH_MTX_NAME(name,mtx)),mtx_plain) == thrd_success); \
  }
#define SQUASH_MTX_LOCK(name) do{                                               \
    call_once (&(SQUASH_MTX_NAME(name,init_flag)), SQUASH_MTX_NAME(name,init)); \
    assert (mtx_lock (&(SQUASH_MTX_NAME(name,mtx))) == thrd_success);   \
  } while(0);
#define SQUASH_MTX_UNLOCK(name) do{                                     \
    assert (mtx_unlock (&(SQUASH_MTX_NAME(name,mtx))) == thrd_success); \
  } while(0);

#endif /* SQUASH_MTX_INTERNAL_H */
