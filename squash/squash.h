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

#ifndef SQUASH_H
#define SQUASH_H

#define SQUASH_H_INSIDE

#include <stddef.h>
#include <stdint.h>

#ifdef  __cplusplus
#  define SQUASH_BEGIN_DECLS extern "C" {
#  define SQUASH_END_DECLS }
#else
#  define SQUASH_BEGIN_DECLS
#  define SQUASH_END_DECLS
#endif

#ifndef SQUASH_API
#  define SQUASH_API extern
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#  define SQUASH_ARRAY_PARAM(name) name
#else
#  define SQUASH_ARRAY_PARAM(name)
#endif

#include <squash/version.h>

#include "status.h"
#include "types.h"
#include "object.h"
#include "options.h"
#include "stream.h"
#include "license.h"
#include "codec.h"
#include "plugin.h"
#include "context.h"

#undef SQUASH_H_INSIDE

#endif /* SQUASH_H */
