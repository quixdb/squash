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
/* IWYU pragma: private, include <squash/squash.h> */

#ifndef SQUASH_MEMORY_H
#define SQUASH_MEMORY_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#if defined(__GNUC__)
#define SQUASH_MALLOC __attribute__((__malloc__))
#else
#define SQUASH_MALLOC
#endif

SQUASH_BEGIN_DECLS

typedef struct SquashMemoryFuncs_ {
  void* (* malloc)                (size_t size);
  void* (* realloc)               (void* ptr, size_t size);
  void* (* calloc)                (size_t nmemb, size_t size);
  void  (* free)                  (void* ptr);

  void* (* aligned_alloc)         (size_t alignment, size_t size);
  void  (* aligned_free)          (void* ptr);
} SquashMemoryFuncs;

SQUASH_API void  squash_set_memory_functions (SquashMemoryFuncs memfn);

SQUASH_MALLOC
SQUASH_API void* squash_malloc               (size_t size);
SQUASH_API void* squash_realloc              (void* ptr, size_t size);
SQUASH_API void* squash_calloc               (size_t nmemb, size_t size);
SQUASH_API void  squash_free                 (void* ptr);
SQUASH_API void* squash_aligned_alloc        (size_t alignment, size_t size);
SQUASH_API void  squash_aligned_free         (void* ptr);

SQUASH_END_DECLS

#endif /* SQUASH_MEMORY_H */
