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

#ifndef SQUASH_STATUS_H
#define SQUASH_STATUS_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

SQUASH_BEGIN_DECLS

typedef enum {
  SQUASH_OK                    =  1,
  SQUASH_PROCESSING            =  2,
  SQUASH_END_OF_STREAM         =  3,

  SQUASH_FAILED                = -1,
  SQUASH_UNABLE_TO_LOAD        = -2,
  SQUASH_BAD_PARAM             = -3,
  SQUASH_BAD_VALUE             = -4,
  SQUASH_MEMORY                = -5,
  SQUASH_BUFFER_FULL           = -6,
  SQUASH_BUFFER_EMPTY          = -7,
  SQUASH_STATE                 = -8,
  SQUASH_INVALID_OPERATION     = -9,
  SQUASH_NOT_FOUND             = -10,
  SQUASH_INVALID_BUFFER        = -11,
  SQUASH_IO                    = -12,
  SQUASH_RANGE                 = -13
} SquashStatus;

SQUASH_API const char* squash_status_to_string (SquashStatus status);

SQUASH_API SquashStatus squash_error (SquashStatus status);

SQUASH_END_DECLS

#endif /* SQUASH_STATUS_H */
