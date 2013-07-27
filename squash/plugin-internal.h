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
 *   Evan Nemerson <evan@coeus-group.com>
 */

#ifndef __SQUASH_PLUGIN_INTERNAL_H__
#define __SQUASH_PLUGIN_INTERNAL_H__

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

SQUASH_BEGIN_DECLS

SquashPlugin*   squash_plugin_new        (char* name, char* directory, SquashContext* context);
void            squash_plugin_add_codec  (SquashPlugin* plugin, SquashCodec* codec);
SquashStatus    squash_plugin_load       (SquashPlugin* plugin);
SquashStatus    squash_plugin_init_codec (SquashPlugin* plugin, SquashCodec* codec, SquashCodecFuncs* funcs);
int             squash_plugin_compare    (SquashPlugin* a, SquashPlugin* b);

SQUASH_TREE_PROTOTYPES(_SquashPlugin, tree)
SQUASH_TREE_DEFINE(_SquashPlugin, tree)

SQUASH_END_DECLS

#endif /* __SQUASH_PLUGIN_INTERNAL_H__ */
