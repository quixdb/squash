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
/* IWYU pragma: private, include <squash/internal.h> */

#ifndef SQUASH_PLUGIN_INTERNAL_H
#define SQUASH_PLUGIN_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

SQUASH_BEGIN_DECLS

SQUASH_NONNULL(1, 2, 3)
SquashPlugin*   squash_plugin_new        (char* name, char* directory, SquashContext* context);
SQUASH_NONNULL(1, 2)
void            squash_plugin_add_codec  (SquashPlugin* plugin, SquashCodec* codec);
SQUASH_NONNULL(1)
SquashStatus    squash_plugin_load       (SquashPlugin* plugin);
SQUASH_NONNULL(1, 2, 3)
SquashStatus    squash_plugin_init_codec (SquashPlugin* plugin, SquashCodec* codec, SquashCodecImpl* impl);
SQUASH_NONNULL(1, 2)
int             squash_plugin_compare    (SquashPlugin* a, SquashPlugin* b);

SQUASH_TREE_PROTOTYPES(_SquashPlugin, tree)
SQUASH_TREE_DEFINE(_SquashPlugin, tree)

SQUASH_END_DECLS

#endif /* SQUASH_PLUGIN_INTERNAL_H */
