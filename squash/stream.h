/* Copyright (c) 2013-2015 The Squash Authors
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

#ifndef SQUASH_STREAM_H
#define SQUASH_STREAM_H

#include <squash/squash.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

SQUASH_BEGIN_DECLS

typedef struct SquashStreamPrivate_ SquashStreamPrivate;

typedef enum {
  SQUASH_STREAM_COMPRESS = 1,
  SQUASH_STREAM_DECOMPRESS = 2
} SquashStreamType;

typedef enum {
  SQUASH_STREAM_STATE_IDLE = 0,
  SQUASH_STREAM_STATE_RUNNING = 1,
  SQUASH_STREAM_STATE_FLUSHING = 2,
  SQUASH_STREAM_STATE_FINISHING = 3,
  SQUASH_STREAM_STATE_FINISHED = 4
} SquashStreamState;

typedef enum {
  SQUASH_OPERATION_PROCESS = 1,
  SQUASH_OPERATION_FLUSH = 2,
  SQUASH_OPERATION_FINISH = 3,
  SQUASH_OPERATION_TERMINATE = 4
} SquashOperation;

struct SquashStream_ {
  SquashObject base_object;
  SquashStreamPrivate* priv;

  const uint8_t* next_in;
  size_t avail_in;
  size_t total_in;

  uint8_t* next_out;
  size_t avail_out;
  size_t total_out;

  SquashCodec* codec;
  SquashOptions* options;
  SquashStreamType stream_type;
  SquashStreamState state;

  void* user_data;
  SquashDestroyNotify destroy_user_data;
};

SQUASH_SENTINEL
SQUASH_NONNULL(1)
SQUASH_API SquashStream*   squash_stream_new                    (SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 ...);
SQUASH_NONNULL(1, 3)
SQUASH_API SquashStream*   squash_stream_newv                   (SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 va_list options);
SQUASH_NONNULL(1)
SQUASH_API SquashStream*   squash_stream_newa                   (SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 const char* const* keys,
                                                                 const char* const* values);
SQUASH_NONNULL(1)
SQUASH_API SquashStream*   squash_stream_new_with_options       (SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 SquashOptions* options);

SQUASH_NONNULL(1)
SQUASH_API SquashStatus    squash_stream_process                (SquashStream* stream);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus    squash_stream_flush                  (SquashStream* stream);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus    squash_stream_finish                 (SquashStream* stream);

SQUASH_NONNULL(1, 2)
SQUASH_API void            squash_stream_init                   (void* stream,
                                                                 SquashCodec* codec,
                                                                 SquashStreamType stream_type,
                                                                 SquashOptions* options,
                                                                 SquashDestroyNotify destroy_notify);
SQUASH_NONNULL(1)
SQUASH_API void            squash_stream_destroy                (void* stream);

SQUASH_END_DECLS

#endif /* SQUASH_STREAM_H */
