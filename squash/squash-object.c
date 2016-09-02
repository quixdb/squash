/* Copyright (c) 2013-2016 The Squash Authors
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

#include <assert.h>
#include "squash-internal.h"
#include <stdbool.h>
#include <stddef.h>
#include "portable-snippets/atomic/atomic.h"

typedef struct {
	psnip_atomic_int32 ref_count;
	psnip_atomic_int32 is_floating;
	SquashDestroyNotify destroy_notify;
} SquashObject;

static const size_t squash_object_offset = (sizeof(SquashObject) + (sizeof(SquashObject) % SQUASH_MAX_ALIGNMENT));
#define SQUASH_OBJECT_TO_PTR(obj) ((void*)(((uintptr_t) obj) + squash_object_offset))
#define SQUASH_PTR_TO_OBJECT(ptr) ((SquashObject*)(((uintptr_t) ptr) - squash_object_offset))

/**
 * @defgroup SquashObject SquashObject
 * @brief Base object for several Squash types.
 *
 * @ref SquashObject is designed to provide a lightweight reference
 * counting.  It serves as a base class for other types in Squash.
 *
 * @section Creating a reference-counted instance
 *
 * Creating a reference-counted instance is very easy; you need only
 * call @ref squash_object_alloc, which behaves much like malloc but
 * takes two additional parameters: is_floating and destroy_notify.
 *
 * We'll get back to is_floating in a bit.  destroy_notify is just a
 * callback where you destroy your struct members.  A simple example:
 *
 * @code
 * struct MyObject {
 *   char* greeting;
 *   int x;
 * }
 *
 * static void my_object_destroy(void* ptr) {
 *   struct MyObject* obj = (struct MyObject*) ptr;
 *   free(obj->greeting);
 * }
 *
 * struct MyObject*
 * my_object_new(char* greeting) {
 *   struct MyObject* obj = squash_object_alloc(sizeof(struct MyObject), false, my_object_destroy);
 *   obj->greeting = strdup(greeting);
 * }
 * @endcode
 *
 * Consumers can simply call @ref squash_object_ref and @ref
 * squash_object_unref to increase or decrease the reference count.
 * Once the count reaches 0, the destroy notify is called and the
 * storage is deallocated.
 *
 * @{
 */

/**
 * @typedef SquashDestroyNotify
 * @brief Callback to be invoked when information @a data is no longer
 *   needed.
 *
 * When you are not subclassing @ref SquashObject, ::SquashDestroyNotify is
 * used almost exclusively for memory management, most simply by
 * passing free().
 */

/**
 * @brief Create a new reference-counted instance
 *
 * @param size Storage size
 * @param is_floating Whether the initial reference is floating
 * @param destroy_notify Callback to invoke when the instance is destroyed
 */
void*
squash_object_alloc(size_t size, bool is_floating, SquashDestroyNotify destroy_notify) {
	SquashObject* obj = squash_malloc(squash_object_offset + size);
	obj->ref_count = 1;
	obj->is_floating = is_floating;
	obj->destroy_notify = destroy_notify;
	psnip_atomic_fence();

	return SQUASH_OBJECT_TO_PTR(obj);
}

/**
 * @brief Increment the reference count on an object.
 *
 * @param ptr The object to increase the reference count of.
 * @return The object which was passed in.
 */
void*
squash_object_ref (void* ptr) {
	if (HEDLEY_UNLIKELY(ptr == NULL))
		return NULL;

	SquashObject* obj = SQUASH_PTR_TO_OBJECT(ptr);

  const psnip_nonatomic_int32 is_floating = 1;
  if (HEDLEY_UNLIKELY(psnip_atomic_int32_compare_exchange(&(obj->is_floating), &is_floating, 0))) {
    return ptr;
  }

	psnip_nonatomic_int32 ref_count = psnip_atomic_int32_add(&(obj->ref_count), 1);
	assert(ref_count > 0);

	return ptr;
}

/**
 * @brief Sink a floating reference if one exists.
 *
 * If a floating reference exists on the object, sink it.
 *
 * For a description of how floating references work, see <a href
 * ="https://developer.gnome.org/gobject/stable/gobject-The-Base-Object-Type.html#floating-ref">GObject's
 * documentation of the concept</a>.  The implementation here is
 * different, but the concept remains the same.
 *
 * @param obj The object to sink the floating reference on.
 * @return The object which was passed in.
 */
void*
squash_object_ref_sink (void* ptr) {
	if (HEDLEY_UNLIKELY(ptr == NULL))
		return NULL;

	SquashObject* obj = SQUASH_PTR_TO_OBJECT(ptr);

  const psnip_nonatomic_int32 is_floating = 1;
  if (HEDLEY_UNLIKELY(psnip_atomic_int32_compare_exchange(&(obj->is_floating), &is_floating, 0)))
    return ptr;

	return ptr;
}

/**
 * @brief Decrement the reference count on an object.
 *
 * Once the reference count reaches 0 the object will be freed.
 *
 * @param ptr The object to decrease the reference count of.
 * @return NULL
 */
void*
squash_object_unref (void* ptr) {
	if (HEDLEY_UNLIKELY(ptr == NULL))
		return NULL;

	SquashObject* obj = SQUASH_PTR_TO_OBJECT(ptr);

	const psnip_nonatomic_int32 ref_count = psnip_atomic_int32_sub(&(obj->ref_count), 1);
	if (ref_count == 1) {
		if (obj->destroy_notify != NULL) {
			obj->destroy_notify(ptr);
      squash_free(obj);
		}
	} else {
    assert(ref_count > 1);
  }

	return NULL;
}

/**
 * @brief Get the current reference count of an object.
 *
 * This should really only be used for debugging.
 *
 * @param ptr The object in question.
 * @return The reference count of @a ptr.
 */
unsigned int
squash_object_get_ref_count (void* ptr) {
	if (HEDLEY_UNLIKELY(ptr == NULL))
		return 0;

	return (unsigned int) psnip_atomic_int32_load(&(SQUASH_PTR_TO_OBJECT(ptr)->ref_count));
}

/**
 * @}
 */
