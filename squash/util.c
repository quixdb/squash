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

#include <squash/config.h>
#include <assert.h>

#include "internal.h"

#if !defined(_WIN32)
#  include <unistd.h>
#  if !defined(_SC_PAGESIZE)
#    include <sys/types.h>
#    include <sys/sysctl.h>
#  endif
#else
#  include <windows.h>
#endif

size_t
squash_get_page_size (void) {
  static size_t page_size = 0;

  if (SQUASH_UNLIKELY(page_size == 0)) {
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = si.dwPageSize;
#elif defined(_SC_PAGESIZE)
    const long ps = sysconf (_SC_PAGESIZE);
    page_size = SQUASH_UNLIKELY(ps == -1) ? 8192 : ((size_t) ps);
#else
    int hw_pagesize[] = { CTL_HW, HW_PAGESIZE };
    unsigned int ps;
    size_t len = sizeof(ps);
    int sres = sysctl (hw_pagesize, sizeof(hw_pagesize) / sizeof(*hw_pagesize), &ps, &len, null, 0);
    page_size = SQUASH_LIKELY(sres == 0) ? (size_t) ps : 8192;
#endif
  }

  return page_size;
}

#if defined(__GNUC__)
__attribute__ ((__const__))
#endif
size_t
squash_npot (size_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
  v |= v >> 32;
#endif
  v++;
  return v;
}
