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

#ifndef SQUASH_H
#define SQUASH_H

#define SQUASH_H_INSIDE

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#if !defined(SQUASH_DISABLE_WIDE_CHAR_API) && !defined(SQUASH_ENABLE_WIDE_CHAR_API)
#  define SQUASH_ENABLE_WIDE_CHAR_API
#endif

#if defined(SQUASH_ENABLE_WIDE_CHAR_API)
#  include <wchar.h>
#endif

#ifdef  __cplusplus
#  define SQUASH_BEGIN_DECLS extern "C" {
#  define SQUASH_END_DECLS }
#else
#  define SQUASH_BEGIN_DECLS
#  define SQUASH_END_DECLS
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) && !defined(__cplusplus)
#  define SQUASH_ARRAY_PARAM(name) name
#else
#  define SQUASH_ARRAY_PARAM(name)
#endif

#if defined(__GNUC__)
#  define squash_assert_unreachable() do { assert(false); __builtin_unreachable(); } while(0)
#elif defined(_MSC_VER)
#  define squash_assert_unreachable() __assume(0)
#else
#  define squash_assert_unreachable() assert(false)
#endif

#if defined(__clang__)
#  if __has_attribute(sentinel)
#    define SQUASH_SENTINEL __attribute__((__sentinel__))
#  else
#    define SQUASH_SENTINEL
#  endif
#elif defined(__GNUC__)
#  define SQUASH_SENTINEL __attribute__((__sentinel__))
#else
#  define SQUASH_SENTINEL
#endif

#if defined(__clang__)
#  if __has_attribute(nonnull)
#    define SQUASH_NONNULL(...) __attribute__((__nonnull__ (__VA_ARGS__)))
#  else
#    define SQUASH_NONNULL(...)
#  endif
#elif !defined(__INTEL_COMPILER) && defined(__GNUC__) && (__GNUC__ >= 5)
#  if defined(NDEBUG)
#    define SQUASH_NONNULL(...) __attribute__((__nonnull__ (__VA_ARGS__)))
#  else
#    define SQUASH_NONNULL(...) __attribute__((__nonnull__ (__VA_ARGS__))) __attribute__((__optimize__("no-isolate-erroneous-paths-attribute")))
#  endif
#elif defined(__GNUC__)
#  define SQUASH_NONNULL(...) __attribute__((__nonnull__ (__VA_ARGS__)))
#else
#  define SQUASH_NONNULL(...)
#endif

#if defined(__clang__) || defined(__GNUC__)
#  define SQUASH_LIKELY(expr) (__builtin_expect ((expr) != 0, 1))
#  define SQUASH_UNLIKELY(expr) (__builtin_expect ((expr) != 0, 0))
#else
#  define SQUASH_LIKELY(expr) (expr)
#  define SQUASH_UNLIKELY(expr) (expr)
#endif

#if defined(_Thread_local) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L))
#  define SQUASH_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#  define SQUASH_THREAD_LOCAL __thread
#elif defined(_WIN32)
#  define SQUASH_THREAD_LOCAL __declspec(thread)
#else
#  define SQUASH_THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  define SQUASH_INTERNAL
#  define SQUASH_EXTERNAL   __declspec(dllexport)
#  define SQUASH_IMPORT     __declspec(dllimport)
#else
#  if defined(__GNUC__) && (__GNUC__ >= 4)
#    define SQUASH_INTERNAL __attribute__ ((visibility ("hidden")))
#    define SQUASH_EXTERNAL __attribute__ ((visibility ("default")))
#  else
#    define SQUASH_INTERNAL
#    define SQUASH_EXTERNAL
#  endif
#  define SQUASH_IMPORT     extern
#endif

#if defined(SQUASH_COMPILATION)
#  define SQUASH_API SQUASH_EXTERNAL
#else
#  define SQUASH_API SQUASH_IMPORT
#endif

#include <squash/squash-version.h>
#include <squash/squash-status.h>
#include <squash/squash-types.h>
#include <squash/squash-object.h>
#include <squash/squash-options.h>
#include <squash/squash-stream.h>
#include <squash/squash-file.h>
#include <squash/squash-license.h>
#include <squash/squash-codec.h>
#include <squash/squash-splice.h>
#include <squash/squash-plugin.h>
#include <squash/squash-memory.h>
#include <squash/squash-context.h>

#undef SQUASH_H_INSIDE

#endif /* SQUASH_H */
