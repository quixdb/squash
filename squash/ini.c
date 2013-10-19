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

#include "ini-internal.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void
squash_ini_parser_init (SquashIniParser* parser,
                        SquashIniParserSectionBeginFunc section_begin,
                        SquashIniParserSectionEndFunc section_end,
                        SquashIniParserKeyFunc key_read,
                        SquashIniParserErrorFunc error_callback,
                        void* user_data,
                        SquashIniParserDestroyNotify user_data_destroy) {
  parser->section_begin = section_begin;
  parser->section_end = section_end;
  parser->key_read = key_read;
  parser->error_callback = error_callback;

  parser->user_data = user_data;
  parser->user_data_destroy = user_data_destroy;
}

void
squash_ini_parser_destroy (SquashIniParser* parser) {
  if (parser != NULL && parser->user_data_destroy != NULL && parser->user_data != NULL) {
    parser->user_data_destroy (parser->user_data);
    parser->user_data_destroy = NULL;
    parser->user_data = NULL;
  }
}

static SquashIniParserError
squash_ini_parser_error (SquashIniParser* parser, SquashIniParserError e, int line_num, int pos, const char* line) {
  return (parser->error_callback != NULL) ? parser->error_callback (parser, e, line_num, pos, line, parser->user_data) : e;
}

const char*
squash_ini_parser_error_to_string (SquashIniParserError e) {
  switch (e) {
    case SQUASH_INI_PARSER_OK:
      return "No error";
      break;
    case SQUASH_INI_PARSER_INVALID_ESCAPE_SEQUENCE:
      return "Invalid escape sequence";
      break;
    case SQUASH_INI_PARSER_UNEXPECTED_CHAR:
      return "Unexpected character";
      break;
    case SQUASH_INI_PARSER_UNEXPECTED_EOF:
      return "Unexpected end of file";
      break;
    case SQUASH_INI_PARSER_ERROR:
    default:
      return "Unknown error";
      break;
  }
}

/**
 * @private
 */
typedef struct _SquashIniString {
  char* str;
  size_t size;
  size_t length;
} SquashIniString;

static void
squash_ini_string_destroy (SquashIniString* str) {
  if (str->str != NULL) {
    free (str->str);
  }
}

static void
squash_ini_string_truncate (SquashIniString* str) {
  if (str->str != NULL) {
    memset (str->str, 0, str->length);
  }
  str->length = 0;
}

static void
squash_ini_string_append_c (SquashIniString* str, char c) {
  if (str != NULL) {
    if (++(str->length) >= str->size ) {
      str->str = (char*) realloc (str->str, str->size = str->length + 1);
      str->str[str->length] = 0;
    }
    str->str[str->length - 1] = c;
  }
}

static void
squash_ini_string_chomp (SquashIniString* str) {
  char* lnws;
  char* p;

  if (str == NULL)
    return;

  p = str->str;
  lnws = p;
  if (p == NULL)
    return;

  for ( p = str->str ; *p != '\0' ; p++ ) {
    if (!isspace (*p)) {
      lnws = p;
    }
  }

  *(lnws + 1) = '\0';
}

static void
squash_ini_string_chug (SquashIniString* str) {
  char* rs;

  if (str == NULL || str->str == NULL)
    return;

  rs = str->str;
  while (isspace (*rs)) {
    rs++;
  }

  if (rs != str->str) {
    memmove (str->str, rs, str->length + 1 - (rs - str->str));
    str->length -= rs - str->str;
  }
}

static void
squash_ini_string_strip (SquashIniString* str) {
  squash_ini_string_chug (str);
  squash_ini_string_chomp (str);
}

static char*
squash_ini_string_get_contents (SquashIniString* str) {
  return (str != NULL) ? ((str->length > 0) ? str->str : NULL) : NULL;
}

#ifndef SQUASH_INI_PARSER_MAX_LINE
#define SQUASH_INI_PARSER_MAX_LINE 4096
#endif

/**
 * @private
 */
typedef enum _SquashIniParserState {
	SQUASH_INI_PARSER_STATE_NONE     = 0,
	SQUASH_INI_PARSER_STATE_SECTION  = 1,
	SQUASH_INI_PARSER_STATE_KEY      = 2,
	SQUASH_INI_PARSER_STATE_DETAIL   = 3,
	SQUASH_INI_PARSER_STATE_VALUE    = 4,
  SQUASH_INI_PARSER_STATE_POS_MASK = 7,
	SQUASH_INI_PARSER_STATE_ESCAPED  = 8
} SquashIniParserState;

int
squash_ini_parser_parse (SquashIniParser* parser, FILE* input) {
  char line[SQUASH_INI_PARSER_MAX_LINE] = { 0, };
  int line_num = 0;
  char* p = line;
  SquashIniString* current = NULL;
  SquashIniString section = { 0, };
  SquashIniString key = { 0, };
  SquashIniString detail = { 0, };
  SquashIniString value = { 0, };
  SquashIniParserState state = SQUASH_INI_PARSER_STATE_NONE;
  int e = SQUASH_INI_PARSER_OK;

  while ( (e >= 0) && (fgets (line, SQUASH_INI_PARSER_MAX_LINE, input)) != NULL ) {
    line_num++;
    state = 0;
    e = SQUASH_INI_PARSER_OK;

    squash_ini_string_truncate (&key);
    squash_ini_string_truncate (&detail);
    squash_ini_string_truncate (&value);

    p = line;

    while (*p != '\0') {
      if ((state & SQUASH_INI_PARSER_STATE_ESCAPED) == SQUASH_INI_PARSER_STATE_ESCAPED) {
        switch (*p) {
          case 'n':
            squash_ini_string_append_c (current, '\n');
            break;
          case 't':
            squash_ini_string_append_c (current, '\t');
            break;
          case 'r':
            squash_ini_string_append_c (current, '\r');
            break;
          case '"':
          case '\\':
          case '[':
          case ']':
          case '=':
            squash_ini_string_append_c (current, *p);
            break;
          default:
            e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_INVALID_ESCAPE_SEQUENCE, line_num, p - line, line);
            break;
        }

        state &= ~SQUASH_INI_PARSER_STATE_ESCAPED;
      } else if (*p == '\\') {
        state |= SQUASH_INI_PARSER_STATE_ESCAPED;
      } else if (*p == '\n') {
        switch (state & SQUASH_INI_PARSER_STATE_POS_MASK) {
          case SQUASH_INI_PARSER_STATE_VALUE:
            if (parser->key_read != NULL) {
              e = parser->key_read (parser,
                                    squash_ini_string_get_contents (&section),
                                    squash_ini_string_get_contents (&key),
                                    squash_ini_string_get_contents (&detail),
                                    squash_ini_string_get_contents (&value),
                                    parser->user_data);
            }
          case SQUASH_INI_PARSER_STATE_NONE:
            break;
          default:
            e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_UNEXPECTED_CHAR, line_num, p - line, line);
            break;
        }
        state = SQUASH_INI_PARSER_STATE_NONE;
      } else if ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_NONE) {
        if (*p == '#') {
          break;
        } else if (*p == '[') {
          if (section.str != NULL) {
            e = parser->section_end (parser, section.str, parser->user_data);
            squash_ini_string_truncate (&section);
          }

          current = &section;
          state = (state & ~SQUASH_INI_PARSER_STATE_POS_MASK) | SQUASH_INI_PARSER_STATE_SECTION;
        } else if (isalnum (*p) || *p == '_') {
          state |= SQUASH_INI_PARSER_STATE_KEY;
          current = &key;
          squash_ini_string_append_c (current, *p);
        } else if (!isspace (*p)) {
          e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_UNEXPECTED_CHAR, line_num, p - line, line);
        }
      } else if ( ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_SECTION) &&
                  (*p == ']') ) {
        state &= ~SQUASH_INI_PARSER_STATE_POS_MASK;
        if (parser->section_begin != NULL) {
          e = parser->section_begin (parser,
                                     squash_ini_string_get_contents (&section),
                                     parser->user_data);
        }
        break;
      } else if ( ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_KEY) &&
                  (*p == '[') ) {
        if (detail.length > 0 ) {
          e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_UNEXPECTED_CHAR, line_num, p - line, line);
        } else {
          state = (state & ~SQUASH_INI_PARSER_STATE_POS_MASK) | SQUASH_INI_PARSER_STATE_DETAIL;
          current = &detail;
        }
      } else if ( ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_DETAIL) &&
                  (*p == ']') ) {
        state = (state & ~SQUASH_INI_PARSER_STATE_POS_MASK) | SQUASH_INI_PARSER_STATE_KEY;
        current = &key;
      } else if ( ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_KEY) &&
                  (*p == '=') ) {
        squash_ini_string_strip (current);

        state = (state & ~SQUASH_INI_PARSER_STATE_POS_MASK) | SQUASH_INI_PARSER_STATE_VALUE;
        current = &value;
      } else if ( ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_KEY) ||
                  ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_DETAIL) ||
                  ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_VALUE) ||
                  ((state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_SECTION) ) {
        if (current->length != 0 || !isspace (*p)) {
          squash_ini_string_append_c (current, *p);
        }
      } else {
        e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_UNEXPECTED_CHAR, line_num, p - line, line);
      }

      p++;
    }
  }

  if ( (state & SQUASH_INI_PARSER_STATE_POS_MASK) == SQUASH_INI_PARSER_STATE_VALUE ) {
    if (parser->key_read != NULL) {
      e = parser->key_read (parser,
                            squash_ini_string_get_contents (&section),
                            squash_ini_string_get_contents (&key),
                            squash_ini_string_get_contents (&detail),
                            squash_ini_string_get_contents (&value),
                            parser->user_data);
    }
  }

  if (section.str != NULL) {
    e = parser->section_end (parser, section.str, parser->user_data);
  }

  if ( (state & SQUASH_INI_PARSER_STATE_POS_MASK) != SQUASH_INI_PARSER_STATE_NONE ) {
    e = squash_ini_parser_error (parser, SQUASH_INI_PARSER_UNEXPECTED_EOF, line_num, p - line, line);
  }

  squash_ini_string_destroy (&section);
  squash_ini_string_destroy (&key);
  squash_ini_string_destroy (&detail);
  squash_ini_string_destroy (&value);

  fclose (input);

  return e;
}
