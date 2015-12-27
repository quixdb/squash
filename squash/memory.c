/* Copyright (c) 2015 The Squash Authors
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

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 200112L)
#  undef _POSIX_C_SOURCE
#endif

#if !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200112L
#endif

#if !defined(_ISOC11_SOURCE)
#  define _ISOC11_SOURCE
#endif

#include "internal.h" /* IWYU pragma: keep */

#if defined(HAVE_ALIGNED_ALLOC) || defined(HAVE_POSIX_MEMALIGN)
#  include <stdlib.h>
#elif defined(HAVE__ALIGNED_MALLOC)
#  include <malloc.h>
#else
#  error No alignmed memory allocation function
#endif

#include <string.h>

#if !defined(HAVE_ALIGNED_ALLOC)
static void*
squash_wrap_aligned_alloc (size_t alignment, size_t size) {
#if defined(HAVE_POSIX_MEMALIGN)
  void* ptr;
  int r = posix_memalign (&ptr, alignment, size);
  if (SQUASH_UNLIKELY(r != 0))
    return (squash_error (SQUASH_MEMORY), NULL);
  return ptr;
#elif defined(HAVE__ALIGNED_MALLOC)
  return _aligned_malloc (size, alignment);
#endif
}
#endif

static SquashMemoryFuncs squash_memfns = {
  malloc,
  realloc,
  free,

#if defined(HAVE_ALIGNED_ALLOC)
  aligned_alloc,
#else
  squash_wrap_aligned_alloc,
#endif

#if defined(HAVE_ALIGNED_ALLOC) || defined(HAVE_POSIX_MEMALIGN)
  free
#else
  _aligned_free
#endif
};

/**
 * @defgroup Memory
 * @brief Low-level memory management
 *
 * With the exception of @ref squash_set_memory_functions, these
 * functions should only be used by plugins.
 *
 * @{
 */

/**
 * @internal
 * @brief Get memory management functions
 *
 * @param[out] memfns Location to store memory management functions
 */
void
squash_get_memory_functions (SquashMemoryFuncs* memfns) {
  *memfns = squash_memfns;
}

/**
 * Set memory management functions for future contexts
 *
 * The `aligned_alloc` and `aligned_free` functions may be `NULL`,
 * but all other functions must be implemented.
 *
 * @param memfn Functions to use to manage memory
 */
void
squash_set_memory_functions (SquashMemoryFuncs memfn) {
  assert (memfn.malloc != NULL);
  assert (memfn.realloc != NULL);
  assert (memfn.free != NULL);
  if (memfn.aligned_alloc == NULL || memfn.aligned_free) {
    assert (memfn.aligned_alloc == NULL);
    assert (memfn.aligned_free == NULL);
  }

  squash_memfns = memfn;
}

void*
squash_malloc (SquashContext* ctx, size_t size) {
  return ctx->memfns.malloc (size);
}

void*
squash_realloc (SquashContext* ctx, void* ptr, size_t size) {
  return ctx->memfns.realloc (ptr, size);
}

void
squash_free (SquashContext* ctx, void* ptr) {
  ctx->memfns.free (ptr);
}

/**
 * Allocate an aligned buffer
 *
 * Memory allocated with this function is assumed not to support
 * reallocation.  In reality, assuming nobody has installed thick
 * wrappers it should be possible to @ref squash_realloc the buffer,
 * but the result is not constrained to the alignment requirements
 * presented to the initial buffer.
 *
 * The value returned by this function must be freed with @ref
 * squash_aligned_free; While some implementations (such as C11's
 * aligned_alloc and the POSIX posix_memalign function) allow values
 * returned by @ref squash_aligned_alloc to be passed directly to
 * @ref squash_free, others (such as Windows' _aligned_malloc) do not.
 * Passing the result of this function to @ref squash_free is
 * considered undefined behavior.
 *
 * @note Values supported for the @a alignment parameter are
 * implementation defined, but a fair assumption is that they must be
 * a power of two and multiple of `sizeof(void*)`.
 *
 * @param ctx The context
 * @param alignment Alignment of the buffer
 * @param size Number of bytes to allocate
 */
void*
squash_aligned_alloc (SquashContext* ctx, size_t alignment, size_t size) {
  if (ctx->memfns.aligned_alloc != NULL) {
    return ctx->memfns.aligned_alloc (alignment, size);
  } else {
    /* This code is only used when people provide custom memory
     * functions but don't bother providing aligned versions.  AFAIK
     * all systems provide a way to get aligned memory, so this
     * shouldn't really be necessary unless you're writing your own
     * malloc implementations (e.g., to track memory usage), or using
     * something weird (PHP's emalloc, Python's PyMem_Malloc, etc.).
     *
     * Note that this function will call squash_malloc() with a much
     * larger buffer than is necessary.  If you have a problem with
     * that then feel free to provide your own aligned_alloc
     * implementation. */

    const size_t ms = size + alignment + sizeof(void*);
    const void* ptr = ctx->memfns.malloc (ms);
    const uintptr_t addr = (uintptr_t) ptr;

    /* Figure out where to put the object.  We want a pointer to the
       real allocation to immediately precede the object (so we can
       recover it later to pass to free()/ */
    size_t padding = alignment - (addr % alignment);
    if (padding < sizeof(void*))
      padding += alignment;
    assert ((padding + size) <= ms);

    memcpy ((void*) (addr + padding - sizeof(void*)), ptr, sizeof(void*));
    return (void*) (addr + padding);
  }

  squash_assert_unreachable ();
}

/**
 * Deallocate an aligned buffer
 *
 * @param ctx The context
 * @param ptr Buffer to deallocate
 */
void squash_aligned_free (SquashContext* ctx, void* ptr) {
  if (ctx->memfns.aligned_free != NULL) {
    return ctx->memfns.aligned_free (ptr);
  } else if (ptr != NULL) {
    ctx->memfns.free ((void*) (((uintptr_t) ptr) - sizeof(void*)));
  }
}

/**
 * @}
 */
