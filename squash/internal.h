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

#ifndef SQUASH_INTERNAL_H
#define SQUASH_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#ifdef SQUASH_H
#error "You must include internal.h before (or instead of) squash.h"
#endif

#ifdef __GNUC__
#  define SQUASH_LIKELY(expr) (__builtin_expect ((expr), true))
#  define SQUASH_UNLIKELY(expr) (__builtin_expect ((expr), false))
#else
#  define SQUASH_LIKELY(expr) (expr)
#  define SQUASH_UNLIKELY(expr) (expr)
#endif

#ifndef SQUASH_API
  #if defined _WIN32 || defined __CYGWIN__
    #ifdef __GNUC__
      #define SQUASH_API __attribute__ ((dllexport))
    #else
      #define SQUASH_API __declspec(dllexport)
    #endif
    #define SQUASH_INTERNAL
  #else
    #if __GNUC__ >= 4
      #define SQUASH_API __attribute__ ((visibility ("default")))
      #define SQUASH_INTERNAL __attribute__ ((visibility ("hidden")))
    #else
      #define SQUASH_API
      #define SQUASH_INTERNAL
    #endif
  #endif
#endif

#include "squash.h"

#include <squash/config.h>

#include "tree-internal.h"
#include "types-internal.h"
#include "context-internal.h"
#include "plugin-internal.h"
#include "codec-internal.h"
#include "slist-internal.h"
#include "buffer-internal.h"
#include "buffer-stream-internal.h"
#include "ini-internal.h"
#include "mtx-internal.h"
#include "stream-internal.h"

#endif /* SQUASH_INTERNAL_H */
