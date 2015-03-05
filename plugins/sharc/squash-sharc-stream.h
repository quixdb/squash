/* Copyright (c) 2013 Evan Nemerson <evan@nemerson.com>
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
 */

#include <stdint.h>

#include "sharc/src/sharc.h"

#ifndef _SQUASH_SHARC_STREAM_H_
#define _SQUASH_SHARC_STREAM_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum _SquashSharcStreamType {
	SQUASH_SHARC_STREAM_COMPRESS,
	SQUASH_SHARC_STREAM_DECOMPRESS
} SquashSharcStreamType;

typedef int SquashSharcStreamStatus;
enum _SquashSharcStreamStatus {
	SQUASH_SHARC_STREAM_OK = 1,
	SQUASH_SHARC_STREAM_PROCESSING = 2,
	SQUASH_SHARC_STREAM_END_OF_STREAM = 3,

	SQUASH_SHARC_STREAM_FAILED = -1,
	SQUASH_SHARC_STREAM_BAD_STREAM = -2,
	SQUASH_SHARC_STREAM_STATE = -3,
	SQUASH_SHARC_STREAM_BUFFER = -4,
	SQUASH_SHARC_STREAM_MEMORY = -5
};

typedef struct _SquashSharcStreamPriv SquashSharcStreamPriv;

typedef struct _SquashSharcStream {
	const uint8_t* next_in;
	size_t avail_in;
	size_t total_in;

	uint8_t* next_out;
	size_t avail_out;
	size_t total_out;

	void* (* alloc_func) (size_t size);
	void (* free_func) (void* ptr);

	SquashSharcStreamType type;
	sharc_byte mode;

	SquashSharcStreamPriv* priv;
} SquashSharcStream;

SquashSharcStreamStatus squash_sharc_stream_init    (SquashSharcStream* stream, SquashSharcStreamType type);
void                    squash_sharc_stream_destroy (SquashSharcStream* stream);
SquashSharcStreamStatus squash_sharc_stream_process (SquashSharcStream* stream);
SquashSharcStreamStatus squash_sharc_stream_flush   (SquashSharcStream* stream);
SquashSharcStreamStatus squash_sharc_stream_finish  (SquashSharcStream* stream);


#ifdef  __cplusplus
}
#endif

#endif /* _SQUASH_SHARC_STREAM_H_ */
