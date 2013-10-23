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


/**
 * @defgroup SquashTimer SquashTimer
 * @brief A cross-platform timer
 *
 * This is just a very simple JSON serializer--it doesn't even do any
 * escaping.  All it is really designed to do is a bit of book-keeping
 * about where you are in the hierarchy and handle the structural
 * formatting accordingly.
 *
 * @{
 */

#include "json-writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef enum {
  SQUASH_JSON_WRITER_MAP,
  SQUASH_JSON_WRITER_ARRAY
} SquashJSONWriterContainerType;

typedef struct _SquashJSONWriterStack SquashJSONWriterStack;

struct _SquashJSONWriterStack {
  SquashJSONWriterStack* next;
  bool first;
  SquashJSONWriterContainerType container_type;
};

struct _SquashJSONWriter {
  FILE* output;
  SquashJSONWriterStack* stack;
  int depth;
};

static void squash_json_writer_push (SquashJSONWriter* writer, SquashJSONWriterContainerType container_type);
static void squash_json_writer_pop (SquashJSONWriter* writer);
static void squash_json_writer_indent (SquashJSONWriter* writer);

static void squash_json_writer_push (SquashJSONWriter* writer, SquashJSONWriterContainerType container_type) {
  SquashJSONWriterStack* current = malloc (sizeof (SquashJSONWriterStack));

  current->next = writer->stack;
  current->first = true;
  current->container_type = container_type;

  writer->stack = current;
  writer->depth++;
}

static void squash_json_writer_pop (SquashJSONWriter* writer) {
  SquashJSONWriterStack* prev = writer->stack;

  writer->stack = prev->next;
  writer->depth--;

  free (prev);
}

/**
 * @brief Create a new JSON writer
 *
 * @param output Stream to write output to
 * @return The new writer
 */
SquashJSONWriter* squash_json_writer_new (FILE* output) {
  SquashJSONWriter* writer = (SquashJSONWriter*) malloc (sizeof (SquashJSONWriter));

  writer->output = output;
  writer->stack = NULL;
  writer->depth = 0;

  fputc ('{', output);
  squash_json_writer_push (writer, SQUASH_JSON_WRITER_MAP);

  return writer;
}

/**
 * @brief Free a JSON writer.
 *
 * If any containers (arrays or maps) are still open it will close
 * them so that the output is a valid file (assuming you've escaped
 * your data properly).
 *
 * @param writer The writer to free
 */
void squash_json_writer_free (SquashJSONWriter* writer) {
  while (writer->stack != NULL) {
    squash_json_writer_end_container (writer);
  }

  free (writer);
}

static void squash_json_writer_indent (SquashJSONWriter* writer) {
  int indent = 0;

  if (!writer->stack->first) {
    fputc (',', writer->output);
  } else {
    writer->stack->first = false;
  }

  fputc ('\n', writer->output);
  while ( indent++ < writer->depth ) {
    fputs ("  ", writer->output);
  }
}

static void squash_json_writer_write_escaped_string (SquashJSONWriter* writer, const char* unescaped) {
  const char* c = unescaped;
  fputc ('"', writer->output);
  while (*c != '\0') {
    switch (*c) {
      case '"':
      case '\\':
        fputc (*c, writer->output);
        break;
    }
    fputc (*c, writer->output);
    c++;
  }
  fputc ('"', writer->output);
}

void squash_json_writer_write_value_string (SquashJSONWriter* writer, const char* value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_ARRAY);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, value);
}

void squash_json_writer_write_value_int (SquashJSONWriter* writer, int value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_ARRAY);

  squash_json_writer_indent (writer);
  fprintf (writer->output, "%d", value);
}

void squash_json_writer_write_value_double (SquashJSONWriter* writer, double value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_ARRAY);

  squash_json_writer_indent (writer);
  fprintf (writer->output, "%g", value);
}

void squash_json_writer_write_element_string_string (SquashJSONWriter* writer, const char* key, const char* value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, key);
  fputs (": ", writer->output);
  squash_json_writer_write_escaped_string (writer, value);
}

void squash_json_writer_write_element_string_int (SquashJSONWriter* writer, const char* key, int value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, key);
  fputs (": ", writer->output);
  fprintf (writer->output, "%d", value);
}

void squash_json_writer_write_element_string_double (SquashJSONWriter* writer, const char* key, double value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, key);
  fputs (": ", writer->output);
  fprintf (writer->output, "%g", value);
}

void squash_json_writer_write_element_int_string (SquashJSONWriter* writer, int key, const char* value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  fputs (": ", writer->output);
  squash_json_writer_write_escaped_string (writer, value);
}

void squash_json_writer_write_element_int_int (SquashJSONWriter* writer, int key, int value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  fputs (": ", writer->output);
  fprintf (writer->output, "%d", value);
}

void squash_json_writer_write_element_int_double (SquashJSONWriter* writer, int key, double value) {
  squash_json_writer_indent (writer);
  fputs (": ", writer->output);
  fprintf (writer->output, "%g", value);
}

void squash_json_writer_write_element_double_string (SquashJSONWriter* writer, double key, const char* value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  fprintf (writer->output, "%g", key);
  fputs (": ", writer->output);
  squash_json_writer_write_escaped_string (writer, value);
}

void squash_json_writer_write_element_double_int (SquashJSONWriter* writer, double key, int value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  fprintf (writer->output, "%g", key);
  fputs (": ", writer->output);
}

void squash_json_writer_write_element_double_double (SquashJSONWriter* writer, double key, double value) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  fprintf (writer->output, "%g", key);
  fputs (": ", writer->output);
  fprintf (writer->output, "%g", value);
}

/**
 * @brief Close a container element
 *
 * @param writer The writer
 */
void squash_json_writer_end_container (SquashJSONWriter* writer) {
  int indent = 1;

  fputc ('\n', writer->output);
  while ( indent++ < writer->depth ) {
    fputs ("  ", writer->output);
  }

  switch (writer->stack->container_type) {
    case SQUASH_JSON_WRITER_MAP:
      fputc ('}', writer->output);
      break;
    case SQUASH_JSON_WRITER_ARRAY:
      fputc (']', writer->output);
      break;
  }

  squash_json_writer_pop (writer);
}

void squash_json_writer_begin_element_string_array (SquashJSONWriter* writer, const char* key) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, key);
  fputs (": [", writer->output);
  squash_json_writer_push (writer, SQUASH_JSON_WRITER_ARRAY);
}

void squash_json_writer_begin_element_string_map (SquashJSONWriter* writer, const char* key) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_MAP);

  squash_json_writer_indent (writer);
  squash_json_writer_write_escaped_string (writer, key);
  fputs (": {", writer->output);
  squash_json_writer_push (writer, SQUASH_JSON_WRITER_MAP);
}

void squash_json_writer_begin_value_array (SquashJSONWriter* writer) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_ARRAY);

  squash_json_writer_indent (writer);
  fputs ("[", writer->output);
  squash_json_writer_push (writer, SQUASH_JSON_WRITER_ARRAY);
}

void squash_json_writer_begin_value_map (SquashJSONWriter* writer) {
  assert (writer != NULL);
  assert (writer->stack->container_type == SQUASH_JSON_WRITER_ARRAY);

  squash_json_writer_indent (writer);
  fputs ("{", writer->output);
  squash_json_writer_push (writer, SQUASH_JSON_WRITER_MAP);
}

/**
 * @}
 */

