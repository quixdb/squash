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

#ifndef __SQUASH_TIMER_H__
#define __SQUASH_TIMER_H__

typedef struct SquashTimer_s SquashTimer;

SquashTimer* squash_timer_new              (void);
void         squash_timer_free             (SquashTimer* timer);

void         squash_timer_start            (SquashTimer* timer);
void         squash_timer_stop             (SquashTimer* timer);
void         squash_timer_reset            (SquashTimer* timer);
void         squash_timer_restart          (SquashTimer* timer);

double       squash_timer_get_elapsed_cpu  (SquashTimer* timer);
double       squash_timer_get_elapsed_wall (SquashTimer* timer);

#endif /* __SQUASH_TIMER_H__ */
