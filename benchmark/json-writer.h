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

#ifndef SQUASH_JSON_WRITER_H
#define SQUASH_JSON_WRITER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct _SquashJSONWriter SquashJSONWriter;

SquashJSONWriter* squash_json_writer_new (FILE* output);
void squash_json_writer_free (SquashJSONWriter* writer);

void squash_json_writer_write_element_string_string (SquashJSONWriter* writer, const char* key, const char* value);
void squash_json_writer_write_element_string_int (SquashJSONWriter* writer, const char* key, int value);
void squash_json_writer_write_element_string_double (SquashJSONWriter* writer, const char* key, double value);
void squash_json_writer_write_element_int_string (SquashJSONWriter* writer, int key, const char* value);
void squash_json_writer_write_element_int_int (SquashJSONWriter* writer, int key, int value);
void squash_json_writer_write_element_int_double (SquashJSONWriter* writer, int key, double value);
void squash_json_writer_write_element_double_string (SquashJSONWriter* writer, double key, const char* value);
void squash_json_writer_write_element_double_int (SquashJSONWriter* writer, double key, int value);
void squash_json_writer_write_element_double_double (SquashJSONWriter* writer, double key, double value);

void squash_json_writer_write_value_string (SquashJSONWriter* writer, const char* value);
void squash_json_writer_write_value_int (SquashJSONWriter* writer, int value);
void squash_json_writer_write_value_double (SquashJSONWriter* writer, double value);

void squash_json_writer_end_container (SquashJSONWriter* writer);
void squash_json_writer_begin_element_string_array (SquashJSONWriter* writer, const char* key);
void squash_json_writer_begin_element_string_map (SquashJSONWriter* writer, const char* key);
void squash_json_writer_begin_value_array (SquashJSONWriter* writer);
void squash_json_writer_begin_value_map (SquashJSONWriter* writer);

#endif /* SQUASH_JSON_WRITER_H */
