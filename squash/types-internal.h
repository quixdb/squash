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

#include <ltdl.h>

#ifndef __SQUASH_TYPES_INTERNAL_H__
#define __SQUASH_TYPES_INTERNAL_H__

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

SQUASH_BEGIN_DECLS

typedef SQUASH_TREE_HEAD(_SquashPluginTree, _SquashPlugin) SquashPluginTree;
typedef SQUASH_TREE_HEAD(_SquashCodecTree, _SquashCodec) SquashCodecTree;
typedef SQUASH_TREE_HEAD(_SquashCodecRefTree, _SquashCodecRef) SquashCodecRefTree;

struct _SquashContext {
  SquashPluginTree plugins;
  SquashCodecRefTree codecs;
  SquashCodecRefTree extensions;
};

struct _SquashPlugin {
  SquashContext* context;

  char* name;
  char* directory;

  lt_dlhandle plugin;

  SquashCodecTree codecs;

  SQUASH_TREE_ENTRY(_SquashPlugin) tree;
};

struct _SquashCodec {
  SquashPlugin* plugin;

  char* name;
  int priority;
  char* extension;

  bool initialized;
  SquashCodecFuncs funcs;

  SQUASH_TREE_ENTRY(_SquashCodec) tree;
};

typedef struct _SquashCodecRef {
  SquashCodec* codec;

  SQUASH_TREE_ENTRY(_SquashCodecRef) tree;
} SquashCodecRef;

SQUASH_END_DECLS

#endif /* __SQUASH_TYPES_INTERNAL_H__ */
