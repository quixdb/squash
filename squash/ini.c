
/* #line 1 "ini.rl" */
/* Copyright (c) 2015 Evan Nemerson <evan@nemerson.com>
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
#endif /* defined(__GNUC__) */


/* #line 166 "ini.rl" */


bool
squash_ini_parse (FILE* input, SquashIniParserCallback callback, void* user_data) {
  int cs, res = 0;
  char cur_block[256];
  const char* p;
  const char* pe;
  const char* eof = NULL;

  assert (callback != NULL);

  bool result = true;
  char* section = malloc (SQUASH_INI_PARSER_MAX_SECTION_LENGTH);
  size_t section_length = 0;
  char* key = malloc (SQUASH_INI_PARSER_MAX_KEY_LENGTH);
  size_t key_length = 0;
  char* value = malloc (SQUASH_INI_PARSER_MAX_VALUE_LENGTH);
  size_t value_length = 0;
  bool value_escaped = false;

  if (section == NULL ||
      key == NULL ||
      value == NULL) {
    result = false;
    goto cleanup;
  }

  
/* #line 76 "ini.c" */
static const char SquashIniParser__actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 7, 1, 
	8, 1, 9, 2, 6, 1
};

static const char SquashIniParser__key_offsets[] = {
	0, 0, 14, 28, 33, 38, 40, 42, 
	43, 43, 52, 62
};

static const char SquashIniParser__trans_keys[] = {
	10, 32, 91, 95, 9, 13, 45, 46, 
	48, 57, 65, 90, 97, 122, 9, 32, 
	61, 95, 11, 13, 45, 46, 48, 57, 
	65, 90, 97, 122, 9, 32, 61, 11, 
	13, 10, 32, 34, 9, 13, 10, 34, 
	34, 92, 10, 95, 45, 46, 48, 57, 
	65, 90, 97, 122, 93, 95, 45, 46, 
	48, 57, 65, 90, 97, 122, 10, 32, 
	91, 95, 9, 13, 45, 46, 48, 57, 
	65, 90, 97, 122, 0
};

static const char SquashIniParser__single_lengths[] = {
	0, 4, 4, 3, 3, 2, 2, 1, 
	0, 1, 2, 4
};

static const char SquashIniParser__range_lengths[] = {
	0, 5, 5, 1, 1, 0, 0, 0, 
	0, 4, 4, 5
};

static const char SquashIniParser__index_offsets[] = {
	0, 0, 10, 20, 25, 30, 33, 36, 
	38, 39, 45, 52
};

static const char SquashIniParser__indicies[] = {
	2, 1, 4, 3, 1, 3, 3, 3, 
	3, 0, 5, 5, 6, 3, 5, 3, 
	3, 3, 3, 0, 5, 5, 6, 5, 
	0, 2, 8, 9, 8, 7, 2, 0, 
	7, 11, 12, 10, 2, 0, 13, 14, 
	14, 14, 14, 14, 0, 16, 15, 15, 
	15, 15, 15, 0, 2, 1, 4, 3, 
	1, 3, 3, 3, 3, 17, 0
};

static const char SquashIniParser__trans_targs[] = {
	0, 1, 11, 2, 9, 3, 4, 5, 
	4, 6, 6, 7, 8, 6, 10, 10, 
	7, 0
};

static const char SquashIniParser__trans_actions[] = {
	1, 0, 15, 5, 0, 0, 0, 7, 
	7, 17, 9, 0, 0, 11, 19, 3, 
	13, 0
};

static const char SquashIniParser__eof_actions[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0
};

static const int SquashIniParser_start = 11;
static const int SquashIniParser_first_final = 11;
static const int SquashIniParser_error = 0;

static const int SquashIniParser_en_main = 11;


/* #line 195 "ini.rl" */
  
/* #line 153 "ini.c" */
	{
	cs = SquashIniParser_start;
	}

/* #line 196 "ini.rl" */

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

    
/* #line 175 "ini.c" */
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = SquashIniParser__trans_keys + _SquashIniParser_key_offsets[cs];
	_trans = SquashIniParser__index_offsets[cs];

	_klen = SquashIniParser__single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = SquashIniParser__range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = SquashIniParser__indicies[_trans];
	cs = SquashIniParser__trans_targs[_trans];

	if ( SquashIniParser__trans_actions[_trans] == 0 )
		goto _again;

	_acts = SquashIniParser__actions + _SquashIniParser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
/* #line 46 "ini.rl" */
	{
    result = false;
    goto cleanup;
  }
	break;
	case 1:
/* #line 51 "ini.rl" */
	{
    if (section_length < (SQUASH_INI_PARSER_MAX_SECTION_LENGTH) - 1) {
      section[section_length++] = (*p);
      section[section_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }
	break;
	case 2:
/* #line 61 "ini.rl" */
	{
    if (key_length < (SQUASH_INI_PARSER_MAX_KEY_LENGTH) - 1) {
      key[key_length++] = (*p);
      key[key_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }
	break;
	case 3:
/* #line 71 "ini.rl" */
	{
    if (value_length != 0 || isgraph((*p))) {
      if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
        value[value_length++] = (*p);
        value[value_length] = '\0';
      } else {
        result = false;
        goto cleanup;
      }
    }
  }
	break;
	case 4:
/* #line 83 "ini.rl" */
	{
    if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
      value[value_length++] = (*p);
      value[value_length] = '\0';
    } else {
      result = false;
      goto cleanup;
    }
  }
	break;
	case 5:
/* #line 93 "ini.rl" */
	{
    if (value_length < (SQUASH_INI_PARSER_MAX_VALUE_LENGTH) - 1) {
      char c = (*p);
      switch ((*p)) {
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
	break;
	case 6:
/* #line 117 "ini.rl" */
	{
    section_length = 0;
  }
	break;
	case 7:
/* #line 121 "ini.rl" */
	{
    if (!callback (section, NULL, NULL, 0, user_data)) {
      result = false;
      goto cleanup;
    }
  }
	break;
	case 8:
/* #line 128 "ini.rl" */
	{
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
	break;
	case 9:
/* #line 156 "ini.rl" */
	{value_escaped = true;}
	break;
/* #line 371 "ini.c" */
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = SquashIniParser__actions + _SquashIniParser_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 0:
/* #line 46 "ini.rl" */
	{
    result = false;
    goto cleanup;
  }
	break;
/* #line 394 "ini.c" */
		}
	}
	}

	_out: {}
	}

/* #line 212 "ini.rl" */
  } while (eof == NULL);

  cleanup:
    if (section != NULL)
      free (section);
    if (key != NULL)
      free (key);
    if (value != NULL)
      free (value);

  return result;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif /* defined(__GNUC__) */
