/* Copyright (c) 2015-2017 The Squash Authors
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
/* IWYU pragma: private, include <squash.h> */

#ifndef SQUASH_SPLICE_H
#define SQUASH_SPLICE_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash.h> can be included directly."
#endif

#include <squash.h>
#include <stddef.h>
#include <stdio.h>

HEDLEY_BEGIN_C_DECLS

HEDLEY_SENTINEL(0)
HEDLEY_NON_NULL(1, 3, 4)
SQUASH_API SquashStatus squash_splice                     (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           FILE* fp_out,
                                                           FILE* fp_in,
                                                           size_t size,
                                                           ...);
HEDLEY_NON_NULL(1, 3, 4)
SQUASH_API SquashStatus squash_splice_with_options        (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           FILE* fp_out,
                                                           FILE* fp_in,
                                                           size_t size,
                                                           SquashOptions* options);
HEDLEY_SENTINEL(0)
HEDLEY_NON_NULL(1, 3, 4)
SQUASH_API SquashStatus squash_splice_custom              (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashWriteFunc write_cb,
                                                           SquashReadFunc read_cb,
                                                           void* user_data,
                                                           size_t size,
                                                           ...);
HEDLEY_NON_NULL(1, 3, 4)
SQUASH_API SquashStatus squash_splice_custom_with_options (SquashCodec* codec,
                                                           SquashStreamType stream_type,
                                                           SquashWriteFunc write_cb,
                                                           SquashReadFunc read_cb,
                                                           void* user_data,
                                                           size_t size,
                                                           SquashOptions* options);

HEDLEY_END_C_DECLS

#endif /* SQUASH_SPLICE_H */
