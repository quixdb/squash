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

#ifndef SQUASH_PLUGIN_H
#define SQUASH_PLUGIN_H

#include <squash/squash.h>
#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

SQUASH_BEGIN_DECLS

SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_plugin_init           (SquashPlugin* plugin);

SQUASH_NONNULL(1)
SQUASH_API const char*    squash_plugin_get_name       (SquashPlugin* plugin);
SQUASH_NONNULL(1)
SQUASH_API SquashLicense* squash_plugin_get_licenses   (SquashPlugin* plugin);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashCodec*   squash_plugin_get_codec      (SquashPlugin* plugin, const char* codec);

typedef void (*SquashPluginForeachFunc) (SquashPlugin* plugin, void* data);

SQUASH_NONNULL(1, 2)
SQUASH_API void           squash_plugin_foreach_codec  (SquashPlugin* plugin, SquashCodecForeachFunc func, void* data);

#if defined _WIN32 || defined __CYGWIN__
#  ifdef __GNUC__
#    define SQUASH_PLUGIN_EXPORT __attribute__ ((dllexport))
#  else
#    define SQUASH_PLUGIN_EXPORT __declspec(dllexport)
#  endif
#else
#  if defined(__GNUC__) && (__GNUC__ >= 4)
#    define SQUASH_PLUGIN_EXPORT __attribute__ ((visibility ("default")))
#  else
#    define SQUASH_PLUGIN_EXPORT
#  endif
#endif

SQUASH_END_DECLS

#endif /* SQUASH_PLUGIN_H */
