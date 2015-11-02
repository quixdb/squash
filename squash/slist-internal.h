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
/* IWYU pragma: private, include <squash/internal.h> */

#ifndef SQUASH_SLIST_INTERNAL_H
#define SQUASH_SLIST_INTERNAL_H

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#include <stdlib.h>

#if defined(_MSC_VER)
#define inline __inline
#endif

SQUASH_BEGIN_DECLS

typedef struct SquashSList_ {
  struct SquashSList_* next;
} SquashSList;

typedef void (*SquashSListForeachDataFunc)(SquashSList* item, void* data);
typedef void (*SquashSListForeachFunc)(SquashSList* item);

static inline void
squash_slist_foreach_data (SquashSList* list, SquashSListForeachDataFunc func, void* data) {
  SquashSList* next;
  for ( next = NULL ; list != NULL ; list = next ) {
    next = list->next;
    func (list, data);
  }
}

static inline void
squash_slist_foreach (SquashSList* list, SquashSListForeachFunc func) {
  SquashSList* next;
  for ( next = NULL ; list != NULL ; list = next ) {
    next = list->next;
    func (list);
  }
}

static inline SquashSList*
squash_slist_get_last (SquashSList* list) {
  if (list != NULL) {
    while (list->next != NULL) {
      list = list->next;
    }
  }

  return list;
}

static inline SquashSList*
squash_slist_append (SquashSList* list, size_t elem_size) {
  SquashSList* item = (SquashSList*) malloc (elem_size);
  item->next = NULL;

  if (list != NULL) {
    squash_slist_get_last (list)->next = item;
  }

  return item;
}

#define SQUASH_SLIST_APPEND(l,T) \
  ((T*) squash_slist_append ((SquashSList*) l, sizeof (T)))

SQUASH_END_DECLS

#endif /* SQUASH_SLIST_INTERNAL_H */
