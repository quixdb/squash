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

#ifndef __SQUASH_OPTIONS_H__
#define __SQUASH_OPTIONS_H__

#if !defined (__SQUASH_H_INSIDE__) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <stdarg.h>

SQUASH_BEGIN_DECLS

struct _SquashOptions {
  SquashObject base_object;

  SquashCodec* codec;
};

SQUASH_API SquashOptions* squash_options_new           (SquashCodec* codec, ...);
SQUASH_API SquashOptions* squash_options_newv          (SquashCodec* codec, va_list options);
SQUASH_API SquashOptions* squash_options_newa          (SquashCodec* codec, const char* const* keys, const char* const* values);

SQUASH_API SquashStatus   squash_options_parse         (SquashOptions* options, ...);
SQUASH_API SquashStatus   squash_options_parsev        (SquashOptions* options, va_list options_list);
SQUASH_API SquashStatus   squash_options_parsea        (SquashOptions* options, const char* const* keys, const char* const* values);

SQUASH_API SquashStatus   squash_options_parse_option  (SquashOptions* options, const char* key, const char* value);

SQUASH_API void        squash_options_init          (void* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
SQUASH_API void        squash_options_destroy       (void* options);

SQUASH_END_DECLS

#endif /* __SQUASH_OPTIONS_H__ */
