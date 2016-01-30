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
/* IWYU pragma: private, include <squash/squash.h> */

#ifndef SQUASH_OPTIONS_H
#define SQUASH_OPTIONS_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <squash/squash.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

SQUASH_BEGIN_DECLS

typedef struct SquashOptionInfoEnumStringMap_ SquashOptionInfoEnumStringMap;
typedef struct SquashOptionInfoEnumString_    SquashOptionInfoEnumString;
typedef struct SquashOptionInfoEnumInt_       SquashOptionInfoEnumInt;
typedef struct SquashOptionInfoRangeInt_      SquashOptionInfoRangeInt;
typedef struct SquashOptionInfoRangeSize_     SquashOptionInfoRangeSize;
typedef struct SquashOptionInfo_              SquashOptionInfo;
typedef union  SquashOptionValue_             SquashOptionValue;

struct SquashOptions_ {
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

struct SquashOptionInfoEnumStringMap_ {
  const char* name;
  int value;
};

struct SquashOptionInfoEnumString_ {
  const SquashOptionInfoEnumStringMap* values;
};

struct SquashOptionInfoEnumInt_ {
  size_t values_length;
  const int* values;
};

struct SquashOptionInfoRangeInt_ {
  int min;
  int max;
  int modulus;
  bool allow_zero;
};

struct SquashOptionInfoRangeSize_ {
  size_t min;
  size_t max;
  size_t modulus;
  bool allow_zero;
};

union SquashOptionValue_ {
  char* string_value;
  int int_value;
  bool bool_value;
  size_t size_value;
};

struct SquashOptionInfo_ {
  const char* name;
  SquashOptionType type;
  union {
    struct SquashOptionInfoEnumString_ enum_string;
    struct SquashOptionInfoEnumInt_ enum_int;
    struct SquashOptionInfoRangeInt_ range_int;
    struct SquashOptionInfoRangeSize_ range_size;
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

SQUASH_API const char*    squash_options_get_string    (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_API bool           squash_options_get_bool      (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_API int            squash_options_get_int       (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_API size_t         squash_options_get_size      (SquashOptions* options, SquashCodec* codec, const char* key);

SQUASH_API const char*    squash_options_get_string_at (SquashOptions* options, SquashCodec* codec, size_t index);
SQUASH_API bool           squash_options_get_bool_at   (SquashOptions* options, SquashCodec* codec, size_t index);
SQUASH_API int            squash_options_get_int_at    (SquashOptions* options, SquashCodec* codec, size_t index);
SQUASH_API size_t         squash_options_get_size_at   (SquashOptions* options, SquashCodec* codec, size_t index);

SQUASH_NONNULL(1, 2, 3)
SQUASH_API SquashStatus   squash_options_set_string    (SquashOptions* options, const char* key, const char* value);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashStatus   squash_options_set_bool      (SquashOptions* options, const char* key, bool value);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashStatus   squash_options_set_int       (SquashOptions* options, const char* key, int value);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashStatus   squash_options_set_size      (SquashOptions* options, const char* key, size_t value);

SQUASH_NONNULL(1, 3)
SQUASH_API SquashStatus   squash_options_set_string_at (SquashOptions* options, size_t index, const char* value);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_set_bool_at   (SquashOptions* options, size_t index, bool value);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_set_int_at    (SquashOptions* options, size_t index, int value);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_set_size_at   (SquashOptions* options, size_t index, size_t value);

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

#if defined(SQUASH_ENABLE_WIDE_CHAR_API)
SQUASH_SENTINEL
SQUASH_NONNULL(1)
SQUASH_API SquashOptions* squash_options_neww          (SquashCodec* codec, ...);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashOptions* squash_options_newvw         (SquashCodec* codec, va_list options);
SQUASH_NONNULL(1)
SQUASH_API SquashOptions* squash_options_newaw         (SquashCodec* codec, const wchar_t* const* keys, const wchar_t* const* values);

SQUASH_NONNULL(2, 3)
SQUASH_API const char*    squash_options_get_stringw   (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_NONNULL(2, 3)
SQUASH_API bool           squash_options_get_boolw     (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_NONNULL(2, 3)
SQUASH_API int            squash_options_get_intw      (SquashOptions* options, SquashCodec* codec, const char* key);
SQUASH_NONNULL(2, 3)
SQUASH_API size_t         squash_options_get_sizew     (SquashOptions* options, SquashCodec* codec, const char* key);

SQUASH_SENTINEL
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_parsew        (SquashOptions* options, ...);
SQUASH_NONNULL(1, 2)
SQUASH_API SquashStatus   squash_options_parsevw       (SquashOptions* options, va_list options_list);
SQUASH_NONNULL(1)
SQUASH_API SquashStatus   squash_options_parseaw       (SquashOptions* options, const wchar_t* const* keys, const wchar_t* const* values);
SQUASH_NONNULL(1, 2, 3)
SQUASH_API SquashStatus   squash_options_parse_optionw (SquashOptions* options, const wchar_t* key, const wchar_t* value);
#endif

SQUASH_END_DECLS

#endif /* SQUASH_OPTIONS_H */
