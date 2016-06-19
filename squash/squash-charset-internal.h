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

#ifndef SQUASH_CHARSET_INTERNAL_H
#define SQUASH_CHARSET_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#include <wchar.h>

SQUASH_BEGIN_DECLS

SQUASH_INTERNAL
const char*  squash_charset_get_locale     (void);
SQUASH_INTERNAL
const char*  squash_charset_get_wide       (void);
SQUASH_NONNULL(2, 3, 5, 6) SQUASH_INTERNAL
bool         squash_charset_convert        (size_t* output_size, char** output, const char* output_charset,
                                            size_t input_size, const char* input, const char* input_charset);
SQUASH_NONNULL(1) SQUASH_INTERNAL
char*        squash_charset_utf8_to_locale (const char* input);
SQUASH_NONNULL(1) SQUASH_INTERNAL
char*        squash_charset_locale_to_utf8 (const char* input);
SQUASH_NONNULL(1) SQUASH_INTERNAL
wchar_t*     squash_charset_locale_to_wide (const char* input);
SQUASH_NONNULL(1) SQUASH_INTERNAL
char*        squash_charset_wide_to_locale (const wchar_t* input);
SQUASH_NONNULL(1) SQUASH_INTERNAL
char*        squash_charset_wide_to_utf8   (const wchar_t* input);
SQUASH_NONNULL(1) SQUASH_INTERNAL
wchar_t*     squash_charset_utf8_to_wide   (const char* input);

SQUASH_END_DECLS

#endif /* SQUASH_CHARSET_INTERNAL_H */
