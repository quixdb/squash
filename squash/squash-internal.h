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

#ifndef SQUASH_INTERNAL_H
#define SQUASH_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#ifdef SQUASH_H
#  error "You must include internal.h instead of squash.h"
#endif

/* Solaris freaks out if you try to use older standards with C99. */
#if defined(__sun) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#  define _XOPEN_SOURCE 600
#endif

#ifndef SQUASH_FILE_BUF_SIZE
#  define SQUASH_FILE_BUF_SIZE ((size_t) (1024 * 1024))
#endif

#include <squash.h>

#include <squash/squash-config.h>
#include <squash/squash-charset-internal.h>
#include <squash/squash-tree-internal.h>
#include <squash/squash-types-internal.h>
#include <squash/squash-memory-internal.h>
#include <squash/squash-context-internal.h>
#include <squash/squash-plugin-internal.h>
#include <squash/squash-codec-internal.h>
#include <squash/squash-slist-internal.h>
#include <squash/squash-buffer-internal.h>
#include <squash/squash-buffer-stream-internal.h>
#include <squash/squash-ini-internal.h>
#include <squash/squash-mtx-internal.h>
#include <squash/squash-stream-internal.h>
#include <squash/squash-util-internal.h>
#if !defined(_WIN32)
#  include <squash/squash-mapped-file-internal.h>
#endif

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strtok_r strtok_s
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif

#endif /* SQUASH_INTERNAL_H */
