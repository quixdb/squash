/* Copyright (c) 2015-2018 The Squash Authors
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

#include "squash-internal.h" /* IWYU pragma: keep */

#if defined(HAVE_ALIGNED_ALLOC) || defined(HAVE_POSIX_MEMALIGN)
#  include <stdlib.h>
#elif defined(HAVE__ALIGNED_MALLOC)
#  include <malloc.h>
#elif defined(HAVE___MINGW_ALIGNED_MALLOC)
#  include <malloc.h>
#  define _aligned_malloc __mingw_aligned_malloc
#  define _aligned_realloc __mingw_aligned_realloc
#  define _aligned_free __mingw_aligned_free
#  define HAVE__ALIGNED_MALLOC
#else
#  error No aligned memory allocation function found
#endif

#include <string.h>

#if !defined(HAVE_ALIGNED_ALLOC)
static void*
squash_wrap_aligned_alloc (size_t alignment, size_t size) {
#if defined(HAVE_POSIX_MEMALIGN)
  void* ptr;
  int r = posix_memalign (&ptr, alignment, size);
  if (HEDLEY_UNLIKELY(r != 0))
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
  calloc,
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

static void*
squash_wrap_calloc (size_t nmemb, size_t size) {
  const size_t s = nmemb * size;
  void* ptr = squash_memfns.malloc (s);
  if (ptr != NULL)
    memset (ptr, 0, s);
  return ptr;
}

static void*
squash_wrap_malloc (size_t size) {
  return squash_memfns.calloc (1, size);
}

static void*
squash_align (void* ptr, size_t alignment) {
  assert (alignment > 0);
  size_t delta = (uintptr_t) ptr % alignment;
  if (delta > 0) {
    delta = alignment - delta;
  }
  return (void*) ((unsigned char*) ptr + delta);
}

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
 * Set memory management functions
 *
 * The `aligned_alloc` and `aligned_free` functions may be `NULL`, as
 * well as either `malloc` or `calloc`.  Other callbacks require a
 * value.
 *
 * @note If you choose to call this function then you must do so
 * before *any* other function in the Squash, or your program will
 * likely crash (due to attempting to free a buffer allocated with the
 * standard allocator using your non-standard free function).
 *
 * @note While Squash itself does not call other memory management
 * functions (such as malloc and free) directly, we can't make any
 * promises about plugins or third-party libraries.  We try to make
 * sure as many as possible support custom memory management functions
 * (often filing bugs and patches upstream), but it is unlikely we
 * will ever reach 100% coverage.
 *
 * @param memfn Functions to use to manage memory
 */
void
squash_set_memory_functions (SquashMemoryFuncs memfn) {
  assert (memfn.malloc != NULL);
  assert (memfn.realloc != NULL);
  assert (memfn.free != NULL);

  if (memfn.calloc == NULL) {
    assert (memfn.malloc != NULL);
    memfn.calloc = squash_wrap_calloc;
  } else if (memfn.malloc == NULL) {
    assert (memfn.calloc != NULL);
    memfn.malloc = squash_wrap_malloc;
  }

  if (memfn.aligned_alloc == NULL || memfn.aligned_free) {
    assert (memfn.aligned_alloc == NULL);
    assert (memfn.aligned_free == NULL);
  }

  squash_memfns = memfn;
}

void*
squash_malloc (size_t size) {
  return squash_memfns.malloc (size);
}

void*
squash_calloc (size_t nmemb, size_t size) {
  return squash_memfns.calloc (nmemb, size);
}

void*
squash_realloc (void* ptr, size_t size) {
  return squash_memfns.realloc (ptr, size);
}

void
squash_free (void* ptr) {
  squash_memfns.free (ptr);
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
squash_aligned_alloc (size_t alignment, size_t size) {
  if (squash_memfns.aligned_alloc != NULL) {
    return squash_memfns.aligned_alloc (alignment, size);
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

    unsigned char* ptr = squash_memfns.malloc (alignment - 1 + sizeof(void*) + size);
    if (ptr == NULL) {
      return NULL;
    }
    unsigned char* aligned_ptr = squash_align (ptr + sizeof(void*), alignment);
    memcpy (aligned_ptr - sizeof(void*), &ptr, sizeof(void*));
    return (void*) aligned_ptr;
  }

  HEDLEY_UNREACHABLE ();
}

/**
 * Deallocate an aligned buffer
 *
 * @param ctx The context
 * @param ptr Buffer to deallocate
 */
void squash_aligned_free (void* ptr) {
  if (squash_memfns.aligned_free != NULL) {
    squash_memfns.aligned_free (ptr);
  } else if (ptr != NULL) {
    void* org_ptr = NULL;
    memcpy (&org_ptr, (unsigned char*) ptr - sizeof(void*), sizeof(void*));
    squash_memfns.free (org_ptr);
  }
}

/**
 * @}
 */
