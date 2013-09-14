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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <squash/squash.h>
#include "timer.h"

static void
print_help_and_exit (int argc, char** argv, int exit_code) {
  fprintf (stderr, "Usage: %s [OPTION]... FILE...\n", argv[0]);
  fprintf (stderr, "Benchmark Squash plugins.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-o outfile    Write data to outfile (default is stdout)\n");
  fprintf (stderr, "\t-h            Print this help screen and exit.\n");
  fprintf (stderr, "\t-c codec      Benchmark the specified codec and exit.\n");
  fprintf (stderr, "\t-f format     Output format.  One of:\n");
  fprintf (stderr, "\t                \"json\" (default)\n");
  fprintf (stderr, "\t                \"csv\"\n");

  exit (exit_code);
}

typedef enum _BenchmarkOutputFormat {
  BENCHMARK_OUTPUT_FORMAT_JSON,
  BENCHMARK_OUTPUT_FORMAT_CSV
} BenchmarkOutputFormat;

struct BenchmarkContext {
  FILE* output;
  FILE* input;
  char* input_name;
  bool first;
  long input_size;
  BenchmarkOutputFormat format;
  SquashTimer* timer;
};

static void
benchmark_context_write_json (struct BenchmarkContext* context, const char* fmt, ...) {
  if (context->format == BENCHMARK_OUTPUT_FORMAT_JSON) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(context->output, fmt, ap);
    va_end(ap);
  }
}

static void
benchmark_context_write_csv (struct BenchmarkContext* context, const char* fmt, ...) {
  if (context->format == BENCHMARK_OUTPUT_FORMAT_CSV) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(context->output, fmt, ap);
    va_end(ap);
  }
}

static void
benchmark_codec (SquashCodec* codec, void* data) {
  struct BenchmarkContext* context = (struct BenchmarkContext*) data;
  FILE* compressed = tmpfile ();
  FILE* decompressed = tmpfile ();

  /* Since we're often running against the source dir, we will pick up
     plugins which have not been compiled.  This should bail us out
     before trying to actually use them. */
  if (squash_codec_init (codec) != SQUASH_OK) {
    return;
  }

  fprintf (stderr, "  %s:%s\n",
           squash_plugin_get_name (squash_codec_get_plugin (codec)),
           squash_codec_get_name (codec));

  if (fseek (context->input, 0, SEEK_SET) != 0) {
    perror ("Unable to seek to beginning of input file");
    exit (-1);
  }

  if (context->first) {
    context->first = false;
  } else {
    benchmark_context_write_json (context, ", ", context->output);
  }

  fputs ("    compressing... ", stderr);
  squash_timer_start (context->timer);
  squash_codec_compress_file_with_options (codec, compressed, context->input, NULL);
  squash_timer_stop (context->timer);
  benchmark_context_write_json (context, "{\n        \"plugin\": \"%s\",\n        \"codec\": \"%s\",\n        \"size\": %ld,\n        \"compress_cpu\": %g,\n        \"compress_wall\": %g,\n",
                                squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                squash_codec_get_name (codec),
                                ftell (compressed),
                                squash_timer_get_elapsed_cpu (context->timer),
                                squash_timer_get_elapsed_wall (context->timer));
  benchmark_context_write_csv (context, "%s,%s,%s,%ld,%ld,%g,%g,",
                               context->input_name,
                               squash_plugin_get_name (squash_codec_get_plugin (codec)),
                               squash_codec_get_name (codec),
                               context->input_size,
                               ftell (compressed),
                               squash_timer_get_elapsed_cpu (context->timer),
                               squash_timer_get_elapsed_wall (context->timer));
  fprintf (stderr, "done (%g CPU, %g wall).\n",
           squash_timer_get_elapsed_cpu (context->timer),
           squash_timer_get_elapsed_wall (context->timer));
  squash_timer_reset (context->timer);

  if (fseek (compressed, 0, SEEK_SET) != 0) {
    perror ("Unable to seek to beginning of compressed file");
    exit (-1);
  }

  fputs ("    decompressing... ", stderr);
  squash_timer_start (context->timer);
  squash_codec_decompress_file_with_options (codec, decompressed, compressed, NULL);
  squash_timer_stop (context->timer);
  benchmark_context_write_json (context, "        \"decompress_cpu\": %g,\n        \"decompress_wall\": %g\n      }",
                                squash_timer_get_elapsed_cpu (context->timer),
                                squash_timer_get_elapsed_wall (context->timer));
  benchmark_context_write_csv (context, "%g,%g\n",
                               squash_timer_get_elapsed_cpu (context->timer),
                               squash_timer_get_elapsed_wall (context->timer));
  fprintf (stderr, "done (%g CPU, %g wall).\n",
           squash_timer_get_elapsed_cpu (context->timer),
           squash_timer_get_elapsed_wall (context->timer));
  squash_timer_reset (context->timer);

  fclose (compressed);
  fclose (decompressed);
}

static void
benchmark_plugin (SquashPlugin* plugin, void* data) {
  squash_plugin_foreach_codec (plugin, benchmark_codec, data);
}

int main (int argc, char** argv) {
  struct BenchmarkContext context = { stdout, NULL, NULL, true, 0, BENCHMARK_OUTPUT_FORMAT_JSON, NULL };
  bool first_input = true;
  int opt;
  int optc;
  SquashCodec* codec = NULL;

  context.timer = squash_timer_new ();

  while ( (opt = getopt(argc, argv, "ho:c:f:")) != -1 ) {
    switch ( opt ) {
      case 'h':
        print_help_and_exit (argc, argv, 0);
        break;
      case 'o':
        context.output = fopen (optarg, "w+");
        if (context.output == NULL) {
          perror ("Unable to open output file");
          return -1;
        }
        break;
      case 'c':
        codec = squash_get_codec ((const char*) optarg);
        if (codec == NULL) {
          fprintf (stderr, "Unable to find codec.\n");
          return -1;
        }
        break;
      case 'f':
        if (strcasecmp ((const char*) optarg, "json") == 0) {
          context.format = BENCHMARK_OUTPUT_FORMAT_JSON;
        } else if (strcasecmp ((const char*) optarg, "csv") == 0) {
          context.format = BENCHMARK_OUTPUT_FORMAT_CSV;
        } else {
          fprintf (stderr, "Invalid output format.\n");
          return -1;
        }
        break;
    }

    optc++;
  }

  if ( optind >= argc ) {
    fputs ("No input files specified.\n", stderr);
    return -1;
  }

  benchmark_context_write_json (&context, "{");
  benchmark_context_write_csv (&context, "Dataset,Plugin,Codec,Uncompressed Size,Compressed Size,Compression CPU Time,Compression Wall Clock Time,Decompression CPU Time,Decompression Wall Clock Time\n");

  while ( optind < argc ) {
    context.input_name = argv[optind];
    context.input = fopen (context.input_name, "r");
    if (context.input == NULL) {
      perror ("Unable to open input data");
      return -1;
    }
    context.first = true;

    if (fseek (context.input, 0, SEEK_END) != 0) {
      perror ("Unable to seek to end of input file");
      exit (-1);
    }
    context.input_size = ftell (context.input);

    fprintf (stderr, "Using %s:\n", context.input_name);
    if (first_input) {
      first_input = false;
      benchmark_context_write_json (&context, "\n");
    } else {
      benchmark_context_write_json (&context, ",\n");
    }

    benchmark_context_write_json (&context, "  \"%s\": {\n    \"uncompressed-size\": %ld,\n    \"data\": [\n      ", context.input_name, context.input_size);

    if (codec == NULL) {
      squash_foreach_plugin (benchmark_plugin, &context);
    } else {
      benchmark_codec (codec, &context);
    }

    
    benchmark_context_write_json (&context, "\n    ]\n  }", context.output);

    optind++;
  }

  benchmark_context_write_json (&context, "\n};\n", context.output);

  squash_timer_free (context.timer);

  return 0;
}
