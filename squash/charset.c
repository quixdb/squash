/* Copyright (c) 2015 The Squash Authors
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

#include "internal.h"

#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <wchar.h>
#include <stdint.h>

#define SQUASH_PREFER_ICONV

#if !defined(_WIN32)
#  include <langinfo.h>
#else
#  include <windows.h>
static char squash_charset_win32_cp[13] = { 0, };
#endif

#if defined(_WIN32) && !defined(strcasecmp)
#  define strcasecmp(a, b) strcmpi(a, b)
#endif

const char*
squash_charset_get_locale (void) {
#if !defined(_WIN32)
  return nl_langinfo (CODESET);
#else
  // TODO: use a once to initialize this
  if (*squash_charset_win32_cp == '\0')
    snprintf (squash_charset_win32_cp, sizeof(squash_charset_win32_cp), "CP%u", GetACP ());
  return squash_charset_win32_cp;
#endif
}

const char*
squash_charset_get_wide (void) {
#if (WCHAR_MAX == INT16_MAX) && defined(__STDC_UTF_16__)
  return "UTF-16";
#elif (WCHAR_MAX == INT32_MAX) && defined(__STDC_UTF_32__)
  return "UTF-32";
#elif defined(_WIN32)
  return "UTF-16";
#elif (WCHAR_MAX == INT32_MAX) && defined(__APPLE__)
  return "UTF-32";
#else
#  error wchar_t encoding is unknown
#endif
}

bool
squash_charset_convert (size_t* output_size, char** output, const char* output_charset,
			size_t input_size, const char* input, const char* input_charset) {
  bool res = true;

  assert (output != NULL);
  assert (output_charset != NULL);
  assert (input != NULL);
  assert (input_charset != NULL);

  if (strcasecmp (output_charset, input_charset) == 0) {
    *output = malloc (input_size);
    if (*output == NULL)
      return false;

    memcpy (*output, input, input_size);
    if (output_size != NULL)
      *output_size = input_size;

    return true;
  }

  iconv_t ctx = iconv_open (output_charset, input_charset);
  if (ctx == ((iconv_t) -1))
    return false;

  char* out_start = NULL;
  size_t out_off = 0;
  size_t out_size = input_size;
  const char* in_p = input;
  do {
    out_size *= 2;
    out_start = realloc (out_start, out_size);
    if (out_start == NULL)
      return false;

    char* out_p = out_start + out_off;
    size_t in_rem = input + input_size - in_p;
    size_t out_rem = out_size - out_off;
    size_t r = iconv (ctx, (void*) &in_p, &in_rem, &out_p, &out_rem);
    if (r == ((size_t) -1)) {
      if (errno == E2BIG) {
	out_off = out_p - out_start;
      } else {
	res = false;
	break;
      }
    } else {
      *output = out_start;
      if (output_size != NULL)
	*output_size = out_p - out_start;
      break;
    }
  } while (true);

  iconv_close (ctx);

  if (!res)
    free (out_start);

  return res;
}

char*
squash_charset_utf8_to_locale (const char* input) {
  assert (input != NULL);

  char* output;
  return squash_charset_convert (NULL, &output, squash_charset_get_locale (), strlen (input) + 1, input, "UTF-8") ? output : NULL;
}

char*
squash_charset_locale_to_utf8 (const char* input) {
  assert (input != NULL);

  char* output;
  return squash_charset_convert (NULL, &output, "UTF-8", strlen (input) + 1, input, squash_charset_get_locale ()) ? output : NULL;
}

wchar_t*
squash_charset_locale_to_wide (const char* input) {
  wchar_t* output = NULL;

  assert (input != NULL);

#if defined(SQUASH_PREFER_ICONV)
  return squash_charset_convert (NULL, (char**) &output, squash_charset_get_wide (),
				 strlen (input) + 1, input, squash_charset_get_locale ()) ? output : NULL;
#else
  const size_t s = mbstowcs (NULL, input, 0) + 1;
  if (s == ((size_t) -1))
    return NULL;

  output = calloc (s, sizeof (wchar_t));
  if (output == NULL)
    return NULL;

  const size_t o = mbstowcs (output, input, s);
  assert (o != ((size_t) -1));

  return output;
#endif
}

char*
squash_charset_wide_to_locale (const wchar_t* input) {
  char* output = NULL;

  assert (input != NULL);

#if defined(SQUASH_PREFER_ICONV)
  return squash_charset_convert(NULL, &output, squash_charset_get_locale (),
				(wcslen (input) + 1) * sizeof (wchar_t), (char*) input, squash_charset_get_wide ()) ? output : NULL;
#else
  const size_t s = wcstombs (NULL, input, 0) + 1;
  if (s == ((size_t) -1))
    return NULL;

  output = calloc (s, sizeof (wchar_t));
  if (output == NULL)
    return NULL;

  const size_t o = wcstombs (output, input, s);
  assert (o != ((size_t) -1));

  return output;
#endif
}

char*
squash_charset_wide_to_utf8 (const wchar_t* input) {
  char* output = NULL;

  assert (input != NULL);

  return squash_charset_convert(NULL, &output, "UTF-8",
				(wcslen (input) + 1) * sizeof (wchar_t), (char*) input, squash_charset_get_wide ()) ? output : NULL;
}

wchar_t*
squash_charset_utf8_to_wide (const char* input) {
  wchar_t* output;

  assert (input != NULL);

  return squash_charset_convert(NULL, (char**) &output, squash_charset_get_wide (),
				strlen (input) + 1, input, "UTF-8") ? output : NULL;
}
