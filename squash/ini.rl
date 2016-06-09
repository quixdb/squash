/* Copyright (c) 2015-2016 Evan Nemerson <evan@nemerson.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include <squash/internal.h> /* IWYU pragma: keep */

#define SQUASH_INI_PARSER_MAX_SECTION_LENGTH 1024
#define SQUASH_INI_PARSER_MAX_KEY_LENGTH     1024
#define SQUASH_INI_PARSER_MAX_VALUE_LENGTH   4096

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-variable"
#if __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif
#endif /* defined(__GNUC__) */

%%{

  machine SquashIniParser;

  action on_error {
    result = false;
    goto cleanup;
  }

  action append_section {
    if (section_length < (SQUASH_INI_PARSER_MAX_SECTION_LENGTH) - 1) {
      section[section_length++] = fc;
      section[section_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }

  action append_key {
    if (key_length < (SQUASH_INI_PARSER_MAX_KEY_LENGTH) - 1) {
      key[key_length++] = fc;
      key[key_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }

  action append_value {
    if (value_length != 0 || isgraph(fc)) {
      if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
        value[value_length++] = fc;
        value[value_length] = '\0';
      } else {
        result = false;
        goto cleanup;
      }
    }
  }

  action append_unescaped_value {
    if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
      value[value_length++] = fc;
      value[value_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }

  action append_escaped_value {
    if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
      char c = fc;
      switch (fc) {
        case 'n':
	  c = '\n';
	  break;
        case 't':
	  c = '\t';
	  break;
        case 'r':
	  c = '\r';
	  break;
        case 'f':
	  c = '\f';
	  break;
      }
      value[value_length++] = c;
    } else {
      result = false;
      goto cleanup;
    }
  }

  action begin_section {
    section_length = 0;
  }

  action emit_section {
    if (!callback (section, NULL, NULL, 0, user_data)) {
      result = false;
      goto cleanup;
    }
  }

  action emit {
    key[key_length] = '\0';
    value[value_length] = '\0';
    if (!value_escaped) {
      while (value_length > 0 && isspace (value[value_length - 1]))
        value[--value_length] = '\0';
    }
    if (key_length != 0) {
      if (!callback (section_length > 0 ? section : NULL, key, value, 0, user_data)) {
        result = false;
        goto cleanup;
      }
    }
    key_length = 0;
    value_length = 0;
    value_escaped = false;
  }

  identifier = [a-zA-Z0-9\-_.]+;
  whitespace = space - '\n';
  printable = print - '\n';

  section = '[' identifier >begin_section @append_section %emit_section ']';

  key = identifier @append_key;

  unescaped_value = [^\n"]+ @append_value;

  escaped_value = ('"' (([^"\\]) @append_unescaped_value | ('\\' any) @append_escaped_value)* '"') >{value_escaped = true;};

  value = escaped_value | unescaped_value;

  kv_pair = (key whitespace* '=' whitespace* value?);

  ini = (whitespace* (section | kv_pair)? '\n')* @emit;

  main := ( ini @!on_error );

}%%

bool
squash_ini_parse (FILE* input, SquashIniParserCallback callback, void* user_data) {
  int cs, res = 0;
  char cur_block[256];
  const char* p;
  const char* pe;
  const char* eof = NULL;

  assert (callback != NULL);

  bool result = true;
  char* section = squash_malloc (SQUASH_INI_PARSER_MAX_SECTION_LENGTH);
  size_t section_length = 0;
  char* key = squash_malloc (SQUASH_INI_PARSER_MAX_KEY_LENGTH);
  size_t key_length = 0;
  char* value = squash_malloc (SQUASH_INI_PARSER_MAX_VALUE_LENGTH);
  size_t value_length = 0;
  bool value_escaped = false;

  if (section == NULL ||
      key == NULL ||
      value == NULL) {
    result = false;
    goto cleanup;
  }

  %% write data;
  %% write init;

  do {
    size_t bytes_read = fread (cur_block, 1, sizeof (cur_block), input);
    if (bytes_read == 0) {
      if (!feof (input)) {
        result = false;
	goto cleanup;
      } else {
        eof = cur_block;
      }
    }

    p = cur_block;
    pe = cur_block + bytes_read;

    %% write exec;
  } while (eof == NULL);

  cleanup:
    if (section != NULL)
      squash_free (section);
    if (key != NULL)
      squash_free (key);
    if (value != NULL)
      squash_free (value);

  return result;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif /* defined(__GNUC__) */
