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
/* IWYU pragma: private, include <squash/squash.h> */

#ifndef SQUASH_OPTIONS_H
#define SQUASH_OPTIONS_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <stdarg.h>

SQUASH_BEGIN_DECLS

typedef struct _SquashOptionInfoEnumStringMap SquashOptionInfoEnumStringMap;
typedef struct _SquashOptionInfoEnumString    SquashOptionInfoEnumString;
typedef struct _SquashOptionInfoEnumInt       SquashOptionInfoEnumInt;
typedef struct _SquashOptionInfoRangeInt      SquashOptionInfoRangeInt;
typedef struct _SquashOptionInfoRangeSize     SquashOptionInfoRangeSize;
typedef struct _SquashOptionInfo              SquashOptionInfo;
typedef union  _SquashOptionValue             SquashOptionValue;

struct _SquashOptions {
  SquashObject base_object;

  SquashCodec* codec;

  SquashOptionValue* values;
};

typedef enum {
  SQUASH_OPTION_TYPE_NONE        = 0,
  SQUASH_OPTION_TYPE_BOOL        = 1,
  SQUASH_OPTION_TYPE_STRING      = 2,
  SQUASH_OPTION_TYPE_INT         = 3,
  SQUASH_OPTION_TYPE_SIZE        = 4,

  SQUASH_OPTION_TYPE_ENUM_STRING = (16 | SQUASH_OPTION_TYPE_STRING),
  SQUASH_OPTION_TYPE_ENUM_INT    = (16 | SQUASH_OPTION_TYPE_INT),

  SQUASH_OPTION_TYPE_RANGE_INT   = (32 | SQUASH_OPTION_TYPE_INT),
  SQUASH_OPTION_TYPE_RANGE_SIZE  = (32 | SQUASH_OPTION_TYPE_SIZE),
} SquashOptionType;

struct _SquashOptionInfoEnumStringMap {
  const char* name;
  int value;
};

struct _SquashOptionInfoEnumString {
  const SquashOptionInfoEnumStringMap* values;
};

struct _SquashOptionInfoEnumInt {
  size_t values_length;
  const int* values;
};

struct _SquashOptionInfoRangeInt {
  int min;
  int max;
  int modulus;
  bool allow_zero;
};

struct _SquashOptionInfoRangeSize {
  size_t min;
  size_t max;
  size_t modulus;
  bool allow_zero;
};

union _SquashOptionValue {
  char* string_value;
  int int_value;
  bool bool_value;
  size_t size_value;
};

struct _SquashOptionInfo {
  const char* name;
  SquashOptionType type;
  union {
    struct _SquashOptionInfoEnumString enum_string;
    struct _SquashOptionInfoEnumInt enum_int;
    struct _SquashOptionInfoRangeInt range_int;
    struct _SquashOptionInfoRangeSize range_size;
  } info;
  SquashOptionValue default_value;
};

SQUASH_SENTINEL
SQUASH_NONNULL(1)
SQUASH_API SquashOptions* squash_options_new           (SquashCodec* codec, ...);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashOptions* squash_options_newv          (SquashCodec* codec, va_list options);
SQUASH_NONNULL(1)
SQUASH_API SquashOptions* squash_options_newa          (SquashCodec* codec, const char* const* keys, const char* const* values);

SQUASH_NONNULL(2)
SQUASH_API const char*    squash_options_get_string    (SquashOptions* options, const char* key);
SQUASH_NONNULL(2)
SQUASH_API bool           squash_options_get_bool      (SquashOptions* options, const char* key);
SQUASH_NONNULL(2)
SQUASH_API int            squash_options_get_int       (SquashOptions* options, const char* key);
SQUASH_NONNULL(2)
SQUASH_API size_t         squash_options_get_size      (SquashOptions* options, const char* key);

SQUASH_SENTINEL
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_parse         (SquashOptions* options, ...);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashStatus   squash_options_parsev        (SquashOptions* options, va_list options_list);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_parsea        (SquashOptions* options, const char* const* keys, const char* const* values);

SQUASH_NONNULL(1, 2, 3)
SQUASH_API SquashStatus   squash_options_parse_option  (SquashOptions* options, const char* key, const char* value);

SQUASH_NONNULL(1, 2)
SQUASH_API void           squash_options_init          (void* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
SQUASH_NONNULL(1)
SQUASH_API void           squash_options_destroy       (void* options);

SQUASH_END_DECLS

#endif /* SQUASH_OPTIONS_H */
