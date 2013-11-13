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

#include <assert.h>
#include <strings.h>

#include "internal.h"

/**
 * @var _SquashOptions::base_object
 * @brief Base object.
 */

/**
 * @var _SquashOptions::codec
 * @brief Codec.
 */

/**
 * @defgroup SquashOptions SquashOptions
 * @brief A set of compression/decompression options.
 *
 * @{
 */

/**
 * @struct _SquashOptions
 * @extends _SquashObject
 * @brief A set of compression/decompression options.
 */

/**
 * @brief Parse a single option.
 *
 * @param options The options context.
 * @param key The option key to parse.
 * @param value The option value to parse.
 * @return A status code.
 * @retval SQUASH_OK Option parsed successfully.
 * @retval SQUASH_BAD_PARAM Invalid @a key.
 * @retval SQUASH_BAD_VALUE Invalid @a value
 */
SquashStatus
squash_options_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashCodecFuncs* funcs;

  assert (options != NULL);
  assert (options->codec != NULL);

  if (key == NULL) {
    return SQUASH_OK;
  }

  funcs = squash_codec_get_funcs (options->codec);
  assert (funcs != NULL);

  if (funcs->parse_option != NULL) {
    return funcs->parse_option (options, key, value);
  } else {
    return (strcasecmp (key, "level") == 0) ? SQUASH_OK : SQUASH_BAD_PARAM;
  }
}

/**
 * @brief Parse an array of options.
 *
 * @param options The options context.
 * @param keys The option keys to parse.
 * @param values The option values to parse.
 * @return A status code.
 */
SquashStatus
squash_options_parsea (SquashOptions* options, const char* const* keys, const char* const* values) {
  SquashStatus status = SQUASH_OK;
  int n;

  assert (options != NULL);

  if (keys == NULL || values == NULL)
    return SQUASH_OK;

  for ( n = 0 ; keys[n] != NULL ; n++ ) {
    status = squash_options_parse_option (options, keys[n], values[n]);
    if (status != SQUASH_OK) {
      break;
    }
  }

  return status;
}

/**
 * @brief Parse a va_list of options.
 *
 * @param options - The options context.
 * @param options_list - The options to parse.  See
 *   ::squash_options_parse for a description of the format.
 * @return A status code.
 */
SquashStatus
squash_options_parsev (SquashOptions* options, va_list options_list) {
  const char* key;
  const char* value;
  SquashStatus status = SQUASH_OK;

  assert (options != NULL);

  while ( (key = va_arg (options_list, char*)) != NULL ) {
    value = va_arg (options_list, char*);

    status = squash_options_parse_option (options, key, value);
    if (status != SQUASH_OK)
      break;
  }

  return status;
}

/**
 * @brief Parse a variadic list of options.
 *
 * @param options The options context.
 * @param ... The options to parse.  These should be alternating key
 *   and value pairs of strings, one for each option, followed by
 *   *NULL*.
 * @return A status code.
 */
SquashStatus
squash_options_parse (SquashOptions* options, ...) {
  va_list options_list;
  SquashStatus status;

  va_start (options_list, options);
  status = squash_options_parsev (options, options_list);
  va_end (options_list);

  return status;
}

/**
 * @brief Create a new group of options.
 *
 * @param codec The codec to create the options for.
 * @param ... A variadic list of string key/value pairs followed by *NULL*
 * @return A new option group, or *NULL* on failure.
 */
SquashOptions*
squash_options_new (SquashCodec* codec, ...) {
  va_list options_list;
  SquashOptions* options;

  va_start (options_list, codec);
  options = squash_options_newv (codec, options_list);
  va_end (options_list);

  return options;
}

/**
 * @brief Create a new group of options from a variadic list.
 *
 * @param codec The codec to create the options for.
 * @param options A variadic list of string key/value pairs followed by *NULL*
 * @return A new option group, or *NULL* if @a codec does not accept
 *   any options or could not be loaded.
 */
SquashOptions*
squash_options_newv (SquashCodec* codec, va_list options) {
  SquashOptions* opts = NULL;
  SquashCodecFuncs* funcs;

  assert (codec != NULL);

  funcs = squash_codec_get_funcs (codec);

  if (funcs != NULL && funcs->create_options != NULL) {
    opts = funcs->create_options (codec);
  }

  if (opts != NULL) {
    squash_options_parsev (opts, options);
  }

  return opts;
}

/**
 * @brief Create a new group of options from key and value arrays.
 *
 * @param codec The codec to create the options for.
 * @param keys A *NULL*-terminated array of keys.
 * @param values A *NULL*-terminated array of values.
 * @return A new option group, or *NULL* on failure.
 */
SquashOptions*
squash_options_newa (SquashCodec* codec, const char* const* keys, const char* const* values) {
  SquashOptions* opts = NULL;
  SquashCodecFuncs* funcs;

  assert (codec != NULL);

  funcs = squash_codec_get_funcs (codec);

  if (funcs != NULL && funcs->create_options != NULL) {
    opts = funcs->create_options (codec);
  }

  if (opts != NULL) {
    squash_options_parsea (opts, keys, values);
  }

  return opts;
}

/**
 * @brief Initialize a new %SquashOptions instance.
 *
 * This function should only be used for subclassing.  See
 * ::squash_object_init for more information.
 *
 * @param options The instance to initialize.
 * @param codec The codec to use.
 * @param destroy_notify The function to be squashled when the reference
 *   count reaches 0
 */
void
squash_options_init (void* options,
                  SquashCodec* codec,
                  SquashDestroyNotify destroy_notify) {
  SquashOptions* o;

  assert (options != NULL);
  assert (codec != NULL);

  o = (SquashOptions*) options;

  squash_object_init (o, true, destroy_notify);
  o->codec = codec;
}

/**
 * @brief Destroy a %SquashOptions instance.
 *
 * This function should only be used for subclassing.  See
 * ::squash_object_destroy for more information.
 *
 * @param options The instance to destroy.
 */
void
squash_options_destroy (void* options) {
  SquashOptions* o;

  assert (options != NULL);

  o = (SquashOptions*) options;

  o->codec = NULL;

  squash_object_destroy (o);
}

/**
 * @}
 */
