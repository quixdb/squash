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

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#  define squash_atomic_inc(var) __sync_fetch_and_add(var, 1)
#  define squash_atomic_dec(var) __sync_fetch_and_sub(var, 1)
#  define squash_atomic_cas(var, orig, val) __sync_val_compare_and_swap(var, orig, val)
#elif defined(_WIN32)
#  define squash_atomic_cas(var, orig, val) InterlockedCompareExchange(var, val, orig)
#else
SQUASH_MTX_DEFINE(atomic_ref)

static unsigned int
squash_atomic_cas (volatile unsigned int* var,
                   unsigned int orig,
                   unsigned int val) {
  unsigned int res;

  SQUASH_MTX_LOCK(atomic_ref);
  res = *var;
  if (res == orig)
    *var = val;
  SQUASH_MTX_UNLOCK(atomic_ref);

  return res;
}
#endif

#if !defined(squash_atomic_inc)
static unsigned int
squash_atomic_inc (volatile unsigned int* var) {
  while (true) {
    unsigned int tmp = *var;
    if (squash_atomic_cas (var, tmp, tmp + 1) == tmp) {
      return tmp;
    }
  }
}
#endif /* defined(squash_atomic_inc) */

#if !defined(squash_atomic_dec)
static unsigned int
squash_atomic_dec (volatile unsigned int* var) {
  while (true) {
    unsigned int tmp = *var;
    assert (tmp > 0);
    if (squash_atomic_cas (var, tmp, tmp - 1) == tmp) {
      return tmp;
    }
  }
}
#endif /* defined(squash_atomic_dec) */

/**
 * @var SquashObject_::ref_count
 * @brief The reference count.
 */

/**
 * @var SquashObject_::is_floating
 * @brief Whether or not the object has a floating reference.
 */

/**
 * @var SquashObject_::destroy_notify
 * @brief Function to call when the reference count reaches 0.
 */

/**
 * @defgroup SquashObject SquashObject
 * @brief Base object for several Squash types.
 *
 * @ref SquashObject is designed to provide a lightweight
 * reference-counted type which can be used as a base class for other
 * types in Squash.
 *
 * @section Subclassing Subclassing
 *
 * Subclassing @ref SquashObject is relatively straightforward.  The
 * first step is to embed @ref SquashObject in your object.  Assuming
 * you're inheriting directly from @ref SquashObject, your code would
 * look something like this:
 *
 * @code
 * struct MyObject {
 *   SquashObject base_object;
 *   char* greeting;
 * }
 * @endcode
 *
 * If you are subclassing another type (which inherits, possibly
 * indirectly, from @ref SquashObject) then you should use that type
 * instead.
 *
 * Next, you should to create an *_init function which takes an
 * existing instance of your class, chains up to the *_init function
 * provided by your base class, and initializes any fields you want
 * initialized:
 *
 * @code
 * void
 * my_object_init (MyObject* obj,
 *                 char* greeting,
 *                 SquashDestroyNotify destroy_notify) {
 *   squash_object_init ((SquashObject*) obj, false, destroy_notify);
 *
 *   obj->greeting = strdup (greeting);
 * }
 * @endcode
 *
 * Of course, whatever is created must be destroyed, so you'll also
 * want to create a *_destroy method to be called when the reference
 * count reaches 0.  Destroy any of your fields first, then chain up
 * to the base class' *_destroy function:
 *
 * @code
 * void
 * my_object_destroy (MyObject* obj) {
 *   if (obj->greeting != NULL) {
 *     squash_free (obj->greeting);
 *   }
 *
 *   squash_object_destroy (obj);
 * }
 * @endcode
 *
 * If your class is not abstract (it is meant to be instantiated, not
 * just subclassed), you should create a constructor:
 *
 * @code
 * MyObject*
 * my_object_new (char* greeting) {
 *   MyObject obj;
 *
 *   obj = squash_malloc (sizeof (MyObject));
 *   my_object_init (obj, greeting, (SquashDestroyNotify) my_object_free);
 *
 *   return obj;
 * }
 * @endcode
 *
 * Note that you *must* use @ref squash_malloc to allocate your
 * object; Squash will call @ref squash_free on it later.
 *
 * @{
 */

/**
 * @struct SquashObject_
 * @brief Reference-counting base class for other types
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
 * @brief Increment the reference count on an object.
 *
 * @param obj The object to increase the reference count of.
 * @return The object which was passed in.
 */
void*
squash_object_ref (void* obj) {
  if (obj == NULL)
    return NULL;

  SquashObject* object = (SquashObject*) obj;

  if (object->is_floating != 0) {
    if (squash_atomic_cas (&(object->is_floating), 1, 0) == 0) {
      squash_atomic_inc (&(object->ref_count));
    }
  } else {
    squash_atomic_inc (&(object->ref_count));
  }

  return obj;
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
squash_object_ref_sink (void* obj) {
  SquashObject* object = (SquashObject*) obj;

  if (object == NULL)
    return object;

  return (squash_atomic_cas (&(object->is_floating), 1, 0) == 1) ? obj : NULL;
}

/**
 * @brief Decrement the reference count on an object.
 *
 * Once the reference count reaches 0 the object will be freed.
 *
 * @param obj The object to decrease the reference count of.
 * @return NULL
 */
void*
squash_object_unref (void* obj) {
  if (obj == NULL)
    return NULL;

  SquashObject* object = (SquashObject*) obj;

  unsigned int ref_count = squash_atomic_dec (&(object->ref_count));

  if (ref_count == 1) {
    if (SQUASH_LIKELY(object->destroy_notify != NULL))
      object->destroy_notify (obj);

    squash_free (obj);

    return NULL;
  } else {
    return NULL;
  }
}

/**
 * @brief Get the current reference count of an object.
 *
 * @param obj The object in question.
 * @return The reference count of _obj_.
 */
unsigned int
squash_object_get_ref_count (void* obj) {
  if (obj == NULL)
    return 0;

  return ((SquashObject*) obj)->ref_count;
}

/**
 * @brief Initialize a new object.
 * @protected
 *
 * @warning This function must only be used to implement a subclass
 * of @ref SquashObject.  Objects returned by *_new functions will
 * already be initialized, and you *must* *not* call this function on
 * them; doing so will likely trigger a memory leak.
 *
 * @param obj The object to initialize.
 * @param is_floating Whether or not the object's reference is
 *   floating
 * @param destroy_notify Function to call when the reference count
 *     reaches 0
 */
void
squash_object_init (void* obj, bool is_floating, SquashDestroyNotify destroy_notify) {
  SquashObject* object = (SquashObject*) obj;

  assert (object != NULL);

  object->ref_count = 1;
  object->is_floating = is_floating ? 1 : 0;
  object->destroy_notify = destroy_notify;
}

/**
 * @brief Destroy an object.
 * @protected
 *
 * @warning This function must only be used to implement a subclass of
 * @ref SquashObject.  Each subclass should implement a *_destroy
 * function which should perform any operations needed to destroy
 * their own data and chain up to the *_destroy function of the base
 * class, eventually invoking ::squash_object_destroy.  Invoking this
 * function in any other context is likely to cause a memory leak or
 * crash.
 *
 * @param obj The object to destroy.
 */
void
squash_object_destroy (void* obj) {
  assert (obj != NULL);
}

/**
 * @}
 */
