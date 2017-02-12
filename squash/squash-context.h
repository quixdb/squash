/* Copyright (c) 2013-2017 The Squash Authors
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

#ifndef SQUASH_CONTEXT_H
#define SQUASH_CONTEXT_H

#include <squash.h>
#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash.h> can be included directly."
#endif

HEDLEY_BEGIN_C_DECLS

SQUASH_API void           squash_set_default_search_path          (const char* search_path);
SQUASH_API SquashContext* squash_context_get_default              (void);
HEDLEY_NON_NULL(1, 2)
SQUASH_API SquashPlugin*  squash_context_get_plugin               (SquashContext* context, const char* plugin);
HEDLEY_NON_NULL(1, 2)
SQUASH_API SquashCodec*   squash_context_get_codec                (SquashContext* context, const char* codec);
HEDLEY_NON_NULL(1, 2)
SQUASH_API void           squash_context_foreach_plugin           (SquashContext* context, SquashPluginForeachFunc func, void* data);
HEDLEY_NON_NULL(1, 2)
SQUASH_API void           squash_context_foreach_codec            (SquashContext* context, SquashCodecForeachFunc func, void* data);
HEDLEY_NON_NULL(1, 2)
SQUASH_API SquashCodec*   squash_context_get_codec_from_extension (SquashContext* context, const char* extension);

HEDLEY_NON_NULL(1)
SQUASH_API SquashPlugin*  squash_get_plugin                       (const char* plugin);
HEDLEY_NON_NULL(1)
SQUASH_API SquashCodec*   squash_get_codec                        (const char* codec);
HEDLEY_NON_NULL(1)
SQUASH_API void           squash_foreach_plugin                   (SquashPluginForeachFunc func, void* data);
HEDLEY_NON_NULL(1)
SQUASH_API void           squash_foreach_codec                    (SquashCodecForeachFunc func, void* data);
HEDLEY_NON_NULL(1)
SQUASH_API SquashCodec*   squash_get_codec_from_extension         (const char* extension);

HEDLEY_END_C_DECLS

#endif /* SQUASH_CONTEXT_H */
