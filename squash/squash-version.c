/* Copyright (c) 2015-2016 The Squash Authors
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

/**
 * @defgroup SquashVersion SquashVersion
 * @brief Library version information.
 *
 * @{
 */

/**
 * @def SQUASH_VERSION_MAJOR
 *
 * Major version number
 */

/**
 * @def SQUASH_VERSION_MINOR
 *
 * Minor version number
 */

/**
 * @def SQUASH_VERSION_REVISION
 *
 * Revision version number
 */

/**
 * @def SQUASH_VERSION_API
 *
 * API version
 */

/**
 * @def SQUASH_VERSION_CURRENT
 *
 * Current version, encoded as a single number
 */

/**
 * @brief Get the library version
 *
 * This function will return the version of the library currently in
 * use.  Note that this may be different than the version you compiled
 * against; generally you should only use this function to make sure
 * it is greater than or equal to @ref SQUASH_VERSION_CURRENT, or for
 * reporting purposes.
 *
 * @return the library version
 */

#include "squash-internal.h"

unsigned int
squash_version (void) {
  return SQUASH_VERSION_CURRENT;
}

/**
 * @brief Get the API version
 *
 * Unlike the library version, the API version will only change when
 * backwards-incompatible changes are made (i.e., it should be "1.0"
 * for every 1.x release).  Code linked against a library version not
 * exactly equal to what it was compiled with will almost certainly
 * fail.
 *
 * @return the API version
 */
const char*
squash_version_api (void) {
  return SQUASH_VERSION_API;
}

/**
 * @}
 */
