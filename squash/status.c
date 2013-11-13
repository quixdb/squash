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

#include "internal.h"

/**
 * @defgroup SquashStatus SquashStatus
 * @brief Response status codes.
 *
 * @{
 */

/**
 * @enum SquashStatus_e
 * @brief Status codes
 */

/**
 * @var SquashStreamType_e::SQUASH_OK
 * @brief Operation completed successfully.
 */

/**
 * @var SquashStreamType_e::SQUASH_PROCESSING
 * @brief Operation partially completed.
 */

/**
 * @var SquashStreamType_e::SQUASH_END_OF_STREAM
 * @brief Reached the end of the stream while decoding.
 */

/**
 * @var SquashStreamType_e::SQUASH_FAILED
 * @brief Operation failed.
 */

/**
 * @var SquashStreamType_e::SQUASH_UNABLE_TO_LOAD
 * @brief Unable to load the requested resource.
 */

/**
 * @var SquashStreamType_e::SQUASH_BAD_PARAM
 * @brief One or more of the parameters were not valid.
 */

/**
 * @var SquashStreamType_e::SQUASH_BAD_VALUE
 * @brief One or more parameter values was not valid.
 */

/**
 * @var SquashStreamType_e::SQUASH_MEMORY
 * @brief Not enough memory is available.
 */

/**
 * @var SquashStreamType_e::SQUASH_BUFFER_FULL
 * @brief Insufficient space in buffer.
 */

/**
 * @var SquashStreamType_e::SQUASH_STATE
 * @brief Performing the requested operation from the current state is
 *   not supported.
 */

/**
 * @var SquashStreamType_e::SQUASH_INVALID_OPERATION
 * @brief The requested operation is not available.
 */

/**
 * @var SquashStreamType_e::SQUASH_NOT_FOUND
 * @brief The requested codec could not be found.
 */

/**
 * @brief Get a string representation of a status code.
 *
 * @param status The status.
 * @return A string describing @a status
 */
const char*
squash_status_to_string (SquashStatus status) {
  switch (status) {
    case SQUASH_OK:
      return "Operation completed successfully";
    case SQUASH_PROCESSING:
      return "Operation partially completed";
    case SQUASH_END_OF_STREAM:
      return "End of stream reached";
    case SQUASH_FAILED:
      return "Operation failed";
    case SQUASH_UNABLE_TO_LOAD:
      return "Unable to load the requested resource";
    case SQUASH_BAD_PARAM:
      return "One or more of the parameters were not valid";
    case SQUASH_BAD_VALUE:
      return "One or more parameter values was not valid";
    case SQUASH_MEMORY:
      return "Not enough memory is available";
    case SQUASH_BUFFER_FULL:
      return "Insufficient space in buffer";
    case SQUASH_BUFFER_EMPTY:
      return "Unable to read from buffer";
    case SQUASH_STATE:
      return "Performing the requested operation from the current state is not supported";
    case SQUASH_INVALID_OPERATION:
      return "The requested operation is not available";
    case SQUASH_NOT_FOUND:
      return "The requested codec could not be found";
    default:
      return "Unknown.";
  }
}

/**
 * @}
 */
