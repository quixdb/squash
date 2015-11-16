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

/* For strdup */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <squash/internal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_MSC_VER)
#include <strings.h>
#endif

static void squash_options_free (void* options);

/**
 * @var SquashOptions_::base_object
 * @brief Base object.
 */

/**
 * @var SquashOptions_::codec
 * @brief Codec.
 */

/**
 * @defgroup SquashOptions SquashOptions
 * @brief A set of compression/decompression options.
 *
 * @{
 */

/**
 * @struct SquashOptions_
 * @extends SquashObject_
 * @brief A set of compression/decompression options.
 *
 * @var SquashOptions_::values
 * @brief NULL-terminated array of option values
 */

/**
 * @struct SquashOptionInfoEnumString_
 * @brief A list of strings which are mapped to integer values
 *
 * @var SquashOptionInfoEnumString_::values
 * @brief a NULL terminated list of string and integer pairs
 */

/**
 * @struct SquashOptionInfoEnumStringMap_
 * @brief An item in a map of strings to integer values
 *
 * @var SquashOptionInfoEnumStringMap_::name
 * @brief a string representing the option value
 * @var SquashOptionInfoEnumStringMap_::value
 * @brief an integer representing the option value
 */

/**
 * @struct SquashOptionInfoEnumInt_
 * @brief A list of potential integer values
 *
 * @var SquashOptionInfoEnumInt_::values_length
 * @brief number of integer values understood for this option
 * @var SquashOptionInfoEnumInt_::values
 * @brief array of integer values understood for this option
 */

/**
 * @struct SquashOptionInfoRangeInt_
 * @brief A range of potential integer values
 *
 * @var SquashOptionInfoRangeInt_::min
 * @brief minimum value for this option
 * @var SquashOptionInfoRangeInt_::max
 * @brief maximum value for this option
 * @var SquashOptionInfoRangeInt_::modulus
 * @brief modulus of acceptable values, or 0 to accept all
 * @var SquashOptionInfoRangeInt_::allow_zero
 * @brief whether to allow zero as a value
 *
 * Note that this is in addition to the range, and independent of the
 * modulus.
 */

/**
 * @struct SquashOptionInfoRangeSize_
 * @brief A range of potential size values
 *
 * @var SquashOptionInfoRangeSize_::min
 * @brief minimum value for this option
 * @var SquashOptionInfoRangeSize_::max
 * @brief maximum value for this option
 * @var SquashOptionInfoRangeSize_::modulus
 * @brief modulus of acceptable values, or 0 to accept all
 * @var SquashOptionInfoRangeSize_::allow_zero
 * @brief whether to allow zero as a value
 *
 * Note that this is in addition to the range, and independent of the
 * modulus.
 */

/**
 * @union SquashOptionValue_
 * @brief A value
 *
 * @var SquashOptionValue_::string_value
 * @brief the value as a string
 * @var SquashOptionValue_::int_value
 * @brief the value as an integer
 * @var SquashOptionValue_::bool_value
 * @brief the value as a boolean
 * @var SquashOptionValue_::size_value
 * @brief the value as a size
 */

/**
 * @struct SquashOptionInfo_
 * @brief Information about options which can be passed to a codec
 *
 * @var SquashOptionInfo_::name
 * @brief name of the option
 * @var SquashOptionInfo_::type
 * @brief type of the option
 * @var SquashOptionInfo_::info
 * @brief detailed information about the value
 * @var SquashOptionInfo_::default_value
 * @brief value to use if none is provided by the user
 */

static int
squash_options_find (SquashOptions* options, const char* key) {
  assert (options != NULL);
  assert (key != NULL);

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec);
  unsigned int option_n = 0;

  {
    while (info->name != NULL) {
      if (strcasecmp (key, info->name) == 0)
        return option_n;

      option_n++;
      info++;
    }
  }

  return -1;
}

/**
 * Retrieve the value of a string option
 *
 * @note If the option is not natively a string (e.g., if it is an
 * integer, size, or boolean), it will not be serialized to one.
 *
 * @param options the options to retrieve the value from
 * @param key name of the option to retrieve the value from
 * @returns the value, or *NULL* on failure
 */
const char*
squash_options_get_string (SquashOptions* options, const char* key) {
  if (options == NULL)
    return NULL;

  const int option_n = squash_options_find (options, key);
  if (option_n == -1)
    return NULL;

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec) + option_n;
  const SquashOptionValue* val = &(options->values[option_n]);

  switch ((int) info->type) {
    case SQUASH_OPTION_TYPE_ENUM_STRING:
      return info->info.enum_string.values[val->int_value].name;
    case SQUASH_OPTION_TYPE_STRING:
      return val->string_value;
    default:
      return NULL;
  }

  squash_assert_unreachable ();
}

/**
 * Retrieve the value of a boolean option
 *
 * @param options the options to retrieve the value from
 * @param key name of the option to retrieve the value from
 * @returns the value
 */
bool
squash_options_get_bool (SquashOptions* options, const char* key) {
  if (options == NULL)
    return false;

  const int option_n = squash_options_find (options, key);
  if (option_n == -1)
    return false;

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec) + option_n;
  const SquashOptionValue* val = &(options->values[option_n]);

  switch ((int) info->type) {
    case SQUASH_OPTION_TYPE_BOOL:
      return val->bool_value;
    default:
      return false;
  }

  squash_assert_unreachable ();
}

/**
 * Retrieve the value of an integer option
 *
 * @param options the options to retrieve the value from
 * @param key name of the option to retrieve the value from
 * @returns the value
 */
int
squash_options_get_int (SquashOptions* options, const char* key) {
  if (options == NULL)
    return -1;

  const int option_n = squash_options_find (options, key);
  if (option_n == -1)
    return -1;

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec) + option_n;
  const SquashOptionValue* val = &(options->values[option_n]);

  switch ((int) info->type) {
    case SQUASH_OPTION_TYPE_INT:
    case SQUASH_OPTION_TYPE_ENUM_INT:
    case SQUASH_OPTION_TYPE_RANGE_INT:
      return val->int_value;
    default:
      return -1;
  }

  squash_assert_unreachable ();
}

/**
 * Retrieve the value of a size option
 *
 * @param options the options to retrieve the value from
 * @param key name of the option to retrieve the value from
 * @returns the value
 */
size_t
squash_options_get_size (SquashOptions* options, const char* key) {
  if (options == NULL)
    return 0;

  const int option_n = squash_options_find (options, key);
  if (option_n == -1)
    return 0;

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec) + option_n;
  const SquashOptionValue* val = &(options->values[option_n]);

  switch ((int) info->type) {
    case SQUASH_OPTION_TYPE_SIZE:
    case SQUASH_OPTION_TYPE_RANGE_SIZE:
      return val->size_value;
    default:
      return 0;
  }

  squash_assert_unreachable ();
}

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
  assert (options != NULL);
  assert (key != NULL);
  assert (value != NULL);
  assert (options->codec != NULL);

  const SquashOptionInfo* info = squash_codec_get_option_info (options->codec);

  if (info == NULL)
    return SQUASH_BAD_PARAM;
  else
    assert (options->values != NULL);

  const int option_n = squash_options_find (options, key);
  if (option_n < 0)
    return SQUASH_BAD_PARAM;

  SquashOptionValue* val = &(options->values[option_n]);
  info = info + option_n;

  switch (info->type) {
    case SQUASH_OPTION_TYPE_RANGE_INT:
    case SQUASH_OPTION_TYPE_INT: {
        int res = atoi (value);
        if (info->type == SQUASH_OPTION_TYPE_RANGE_INT) {
          if (SQUASH_UNLIKELY(!((res == 0 && info->info.range_int.allow_zero) ||
                                (res >= info->info.range_int.min && res <= info->info.range_int.max))))
            return squash_error (SQUASH_BAD_VALUE);
        }
        val->int_value = res;
        return SQUASH_OK;
      }
      break;

    case SQUASH_OPTION_TYPE_RANGE_SIZE:
    case SQUASH_OPTION_TYPE_SIZE: {
        char* endptr = NULL;
        unsigned long long int res = strtoull (value, &endptr, 10);

        /* Parse X(KMG)[i[B]] into a size in bytes. */
        if (*endptr != '\0') {
          switch (*endptr++) {
            case 'g':
            case 'G':
              res *= 1024;
              /* Fall through */
            case 'm':
            case 'M':
              res *= 1024;
              /* Fall through */
            case 'k':
            case 'K':
              res *= 1024;
              break;
            default:
              return squash_error (SQUASH_BAD_VALUE);
          }

          if (*endptr != '\0') {
            if (*endptr == 'i' || *endptr == 'I')
              endptr++;

            if (SQUASH_LIKELY(*endptr == 'b' || *endptr == 'B'))
              endptr++;
            else
              return squash_error (SQUASH_BAD_VALUE);

            if (SQUASH_UNLIKELY(*endptr != '\0'))
              return squash_error (SQUASH_BAD_VALUE);
          }
        }

        if (info->type == SQUASH_OPTION_TYPE_RANGE_SIZE) {
          if (SQUASH_UNLIKELY(!((res == 0 && info->info.range_size.allow_zero) ||
                                (res >= info->info.range_size.min && res <= info->info.range_size.max))))
            return squash_error (SQUASH_BAD_VALUE);
        }
        val->size_value = (size_t) res;
        return SQUASH_OK;
      }
      break;

    case SQUASH_OPTION_TYPE_ENUM_STRING: {
        for (unsigned int i = 0 ; info->info.enum_string.values[i].name != NULL ; i++) {
          if (strcasecmp (value, info->info.enum_string.values[i].name) == 0) {
            val->int_value = info->info.enum_string.values[i].value;
            return SQUASH_OK;
          }
        }
        return squash_error (SQUASH_BAD_VALUE);
      }
      break;

    case SQUASH_OPTION_TYPE_BOOL: {
        if (strcasecmp (value, "true") == 0 ||
            strcasecmp (value, "yes") == 0 ||
            strcasecmp (value, "on") == 0 ||
            strcasecmp (value, "t") == 0 ||
            strcasecmp (value, "y") == 0 ||
            strcasecmp (value, "1") == 0) {
          val->bool_value = true;
        } else if (strcasecmp (value, "false") == 0 ||
            strcasecmp (value, "no") == 0 ||
            strcasecmp (value, "off") == 0 ||
            strcasecmp (value, "f") == 0 ||
            strcasecmp (value, "n") == 0 ||
            strcasecmp (value, "0") == 0) {
          val->bool_value = false;
        } else {
          return squash_error (SQUASH_BAD_VALUE);
        }
        return SQUASH_OK;
      }
      break;

    case SQUASH_OPTION_TYPE_ENUM_INT: {
        int res = atoi (value);
        for (unsigned int i = 0 ; i < info->info.enum_int.values_length ; i++) {
          if (res == info->info.enum_int.values[i]) {
            val->int_value = res;
            return SQUASH_OK;
          }
        }
        return squash_error (SQUASH_BAD_VALUE);
      }
      break;

    case SQUASH_OPTION_TYPE_STRING:
      val->string_value = strdup (value);
      break;

    case SQUASH_OPTION_TYPE_NONE:
    default:
      squash_assert_unreachable();
  }

  return squash_error (SQUASH_FAILED);
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

static SquashOptions*
squash_options_create (SquashCodec* codec) {
  SquashOptions* options = malloc (sizeof (SquashOptions));
  squash_options_init (options, codec, squash_options_free);
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

  assert (codec != NULL);

  if (squash_codec_get_option_info (codec) != NULL) {
    opts = squash_options_create (codec);
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

  assert (codec != NULL);

  if (squash_codec_get_option_info (codec) != NULL) {
    opts = squash_options_create (codec);
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

  const SquashOptionInfo* info = squash_codec_get_option_info (codec);
  if (info != NULL) {
    size_t n_options;
    for (n_options = 0 ; info[n_options].name != NULL ; n_options++) { }

    assert (n_options != 0);

    o->values = calloc (n_options, sizeof (SquashOptionValue));
    for (size_t c_option = 0 ; c_option < n_options ; c_option++) {
      switch (info[c_option].type) {
        case SQUASH_OPTION_TYPE_ENUM_STRING:
        case SQUASH_OPTION_TYPE_RANGE_INT:
        case SQUASH_OPTION_TYPE_INT:
        case SQUASH_OPTION_TYPE_ENUM_INT:
          o->values[c_option].int_value = info[c_option].default_value.int_value;
          break;
        case SQUASH_OPTION_TYPE_BOOL:
          o->values[c_option].bool_value = info[c_option].default_value.bool_value;
          break;
        case SQUASH_OPTION_TYPE_SIZE:
        case SQUASH_OPTION_TYPE_RANGE_SIZE:
          o->values[c_option].size_value = info[c_option].default_value.size_value;
          break;
        case SQUASH_OPTION_TYPE_STRING:
          o->values[c_option].string_value = strdup (info[c_option].default_value.string_value);
          break;
        case SQUASH_OPTION_TYPE_NONE:
        default:
          squash_assert_unreachable();
      }
    }
  }
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

  SquashOptionValue* values = o->values;
  if (values != NULL) {
    const SquashOptionInfo* info = squash_codec_get_option_info (o->codec);
    assert (info != NULL);

    for (int i = 0 ; info[i].name != NULL ; i++)
      if (info[i].type == SQUASH_OPTION_TYPE_STRING)
        free (values[i].string_value);

    free (values);
  }

  squash_object_destroy (o);
}

static void
squash_options_free (void* options) {
  squash_options_destroy ((SquashOptions*) options);
  free (options);
}

#if defined(SQUASH_ENABLE_WIDE_CHAR_API)
/**
 * @brief Parse a single option with wide character strings.
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
squash_options_parse_optionw (SquashOptions* options, const wchar_t* key, const wchar_t* value) {
  char* nkey = NULL;
  char* nvalue = NULL;
  SquashStatus res = SQUASH_OK;

  nkey = squash_charset_wide_to_utf8 (key);
  if (SQUASH_UNLIKELY(nkey == NULL)) {
    res = squash_error (SQUASH_FAILED);
    goto finish;
  }

  nvalue = squash_charset_wide_to_utf8 (value);
  if (SQUASH_UNLIKELY(nvalue == NULL)) {
    res = squash_error (SQUASH_FAILED);
    goto finish;
  }

  res = squash_options_parse_option (options, nkey, nvalue);

 finish:
  free (nkey);
  free (nvalue);

  return res;
}

/**
 * @brief Parse an array of wide character options.
 *
 * @param options The options context.
 * @param keys The option keys to parse.
 * @param values The option values to parse.
 * @return A status code.
 */
SquashStatus
squash_options_parseaw (SquashOptions* options, const wchar_t* const* keys, const wchar_t* const* values) {
  SquashStatus status = SQUASH_OK;
  int n;

  assert (options != NULL);

  if (keys == NULL || values == NULL)
    return SQUASH_OK;

  for ( n = 0 ; keys[n] != NULL ; n++ ) {
    status = squash_options_parse_optionw (options, keys[n], values[n]);
    if (status != SQUASH_OK) {
      break;
    }
  }

  return status;
}

/**
 * @brief Parse a va_list of wide-character options.
 *
 * @param options - The options context.
 * @param options_list - The options to parse.  See
 *   ::squash_options_parse for a description of the format.
 * @return A status code.
 */
SquashStatus
squash_options_parsevw (SquashOptions* options, va_list options_list) {
  const wchar_t* key;
  const wchar_t* value;
  SquashStatus status = SQUASH_OK;

  assert (options != NULL);

  while ( (key = va_arg (options_list, wchar_t*)) != NULL ) {
    value = va_arg (options_list, wchar_t*);

    status = squash_options_parse_optionw (options, key, value);
    if (status != SQUASH_OK)
      break;
  }

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
squash_options_neww (SquashCodec* codec, ...) {
  va_list options_list;
  SquashOptions* options;

  va_start (options_list, codec);
  options = squash_options_newvw (codec, options_list);
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
squash_options_newvw (SquashCodec* codec, va_list options) {
  SquashOptions* opts = NULL;

  assert (codec != NULL);

  if (squash_codec_get_option_info (codec) != NULL) {
    opts = squash_options_create (codec);
    squash_options_parsevw (opts, options);
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
squash_options_newaw (SquashCodec* codec, const wchar_t* const* keys, const wchar_t* const* values) {
  SquashOptions* opts = NULL;

  assert (codec != NULL);

  if (squash_codec_get_option_info (codec) != NULL) {
    opts = squash_options_create (codec);
    squash_options_parseaw (opts, keys, values);
  }

  return opts;
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
squash_options_parsew (SquashOptions* options, ...) {
  va_list options_list;
  SquashStatus status;

  va_start (options_list, options);
  status = squash_options_parsevw (options, options_list);
  va_end (options_list);

  return status;
}
#endif

/**
 * @}
 */
