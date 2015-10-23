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
/* IWYU pragma: private, include "internal.h" */

#ifndef SQUASH_TYPES_INTERNAL_H
#define SQUASH_TYPES_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#if defined(_WIN32)
#include <windows.h>
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
  SquashLicense* license;

#if !defined(_WIN32)
  void* plugin;
#else
  HMODULE plugin;
#endif

  SquashCodecTree codecs;

  SQUASH_TREE_ENTRY(_SquashPlugin) tree;
};

struct _SquashCodec {
  SquashPlugin* plugin;

  char* name;
  int priority;
  char* extension;

  bool initialized;
  SquashCodecImpl impl;

  SQUASH_TREE_ENTRY(_SquashCodec) tree;
};

typedef struct _SquashCodecRef {
  SquashCodec* codec;

  SQUASH_TREE_ENTRY(_SquashCodecRef) tree;
} SquashCodecRef;

typedef struct _SquashBuffer {
  uint8_t* data;
  size_t size;
  size_t allocated;
} SquashBuffer;

SQUASH_END_DECLS

#endif /* SQUASH_TYPES_INTERNAL_H */
