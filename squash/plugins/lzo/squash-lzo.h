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

#ifndef __SQUASH_PLUGIN_LZO_H__
#define __SQUASH_PLUGIN_LZO_H__

#include <lzo/lzoconf.h>

typedef struct _SquashLZOCompressor {
  int level;
  size_t work_mem;
  int(* compress) (const lzo_bytep src, lzo_uint src_len,
                   lzo_bytep dst, lzo_uintp dst_len,
                   lzo_voidp wrkmem);
} SquashLZOCompressor;

typedef struct _SquashLZOCodec {
  const char* name;
  size_t work_mem;
  int(* decompress) (const lzo_bytep src, lzo_uint src_len,
                     lzo_bytep dst, lzo_uintp dst_len,
                     lzo_voidp wrkmem);
  const SquashLZOCompressor* compressors;
} SquashLZOCodec;

typedef struct _SquashLZOOptions {
  SquashOptions base_object;

  int level;
} SquashLZOOptions;

typedef struct _SquashLZOStream {
  SquashStream base_object;

  SquashLZOCodec* codec;
  SquashLZOCompressor* compressor;
} SquashLZOStream;

#endif /* __SQUASH_PLUGIN_LZO_H__ */
