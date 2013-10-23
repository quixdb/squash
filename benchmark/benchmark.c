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
#include <assert.h>

#include <squash/squash.h>
#include "timer.h"
#include "json-writer.h"

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
  long input_size;
  BenchmarkOutputFormat format;
  SquashJSONWriter* json;
  SquashTimer* compress_timer;
  SquashTimer* decompress_timer;
};

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
benchmark_report_result (SquashCodec* codec, struct BenchmarkContext* context, FILE* compressed, FILE* decompressed, int level) {
  if (context->format == BENCHMARK_OUTPUT_FORMAT_JSON) {
    squash_json_writer_begin_value_map (context->json);

    if (level >= 0)
      squash_json_writer_write_element_string_int (context->json, "level", level);
    squash_json_writer_write_element_string_int (context->json, "compressed-size", ftell (compressed));
    squash_json_writer_write_element_string_double (context->json, "compress-cpu", squash_timer_get_elapsed_cpu (context->compress_timer));
    squash_json_writer_write_element_string_double (context->json, "compress-wall", squash_timer_get_elapsed_wall (context->compress_timer));
    squash_json_writer_write_element_string_double (context->json, "decompress-cpu", squash_timer_get_elapsed_cpu (context->decompress_timer));
    squash_json_writer_write_element_string_double (context->json, "decompress-wall", squash_timer_get_elapsed_wall (context->decompress_timer));
    squash_json_writer_end_container (context->json);
  } else {
    if (level >= 0) {
      benchmark_context_write_csv (context, "%s,%s,%s,%d,%ld,%ld,%g,%g,%g,%g",
                                   context->input_name,
                                   squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                   squash_codec_get_name (codec),
                                   level,
                                   context->input_size,
                                   ftell (compressed),
                                   squash_timer_get_elapsed_cpu (context->compress_timer),
                                   squash_timer_get_elapsed_wall (context->compress_timer));
    } else {
      benchmark_context_write_csv (context, "%s,%s,%s,,%ld,%ld,%g,%g,%g,%g",
                                   context->input_name,
                                   squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                   squash_codec_get_name (codec),
                                   context->input_size,
                                   ftell (compressed),
                                   squash_timer_get_elapsed_cpu (context->compress_timer),
                                   squash_timer_get_elapsed_wall (context->compress_timer),
                                   squash_timer_get_elapsed_cpu (context->decompress_timer),
                                   squash_timer_get_elapsed_wall (context->decompress_timer));
    }
  }
}

static void
benchmark_codec (SquashCodec* codec, void* data) {
  struct BenchmarkContext* context = (struct BenchmarkContext*) data;
  FILE* compressed;
  FILE* decompressed;
  SquashOptions* opts;
  int level = 0;
  char level_s[4];
  int num_results = 0;

  fprintf (stderr, "%s:%s\n",
           squash_plugin_get_name (squash_codec_get_plugin (codec)),
           squash_codec_get_name (codec));

  if (fseek (context->input, 0, SEEK_SET) != 0) {
    perror ("Unable to seek to beginning of input file");
    exit (-1);
  }

  squash_json_writer_begin_element_string_array (context->json, squash_codec_get_name (codec));

  opts = squash_options_new (codec, NULL);
  if (opts != NULL) {
    squash_object_ref_sink (opts);
    for ( level = 0 ; level <= 999 ; level++ ) {
      snprintf (level_s, 4, "%d", level);
      if (squash_options_parse_option (opts, "level", level_s) == SQUASH_OK) {
        compressed = tmpfile ();
        decompressed = tmpfile ();

        fprintf (stderr, "    level %d: ", level);
        squash_timer_start (context->compress_timer);
        squash_codec_compress_file_with_options (codec, compressed, context->input, opts);
        squash_timer_stop (context->compress_timer);
        assert (ftell (compressed) > 0);
        fprintf (stderr, "compressed (%.4f CPU, %.4f wall, %ld bytes)... ",
                 squash_timer_get_elapsed_cpu (context->compress_timer),
                 squash_timer_get_elapsed_wall (context->compress_timer),
                 ftell (compressed));

        fseek (compressed, 0, SEEK_SET);
        squash_timer_start (context->decompress_timer);
        squash_codec_decompress_file_with_options (codec, decompressed, compressed, opts);
        squash_timer_stop (context->decompress_timer);
        fprintf (stderr, "decompressed (%.6f CPU, %.6f wall).\n",
                 squash_timer_get_elapsed_cpu (context->decompress_timer),
                 squash_timer_get_elapsed_wall (context->decompress_timer));

        fseek (context->input, 0, SEEK_END);
        fseek (compressed, 0, SEEK_END);
        fseek (decompressed, 0, SEEK_END);
        assert (ftell (context->input) == ftell (decompressed));

        benchmark_report_result (codec, context, compressed, decompressed, level);
        num_results++;

        squash_timer_reset (context->compress_timer);
        squash_timer_reset (context->decompress_timer);
        fclose (compressed);
        fclose (decompressed);
        fseek (context->input, 0, SEEK_SET);
      }
    }
    squash_object_unref (opts);
  }

  if (num_results == 0) {
    compressed = tmpfile ();
    decompressed = tmpfile ();

    fprintf (stderr, "    compressing: ");
    squash_timer_start (context->compress_timer);
    squash_codec_compress_file_with_options (codec, compressed, context->input, NULL);
    squash_timer_stop (context->compress_timer);
    fprintf (stderr, "compressed (%.4f CPU, %.4f wall)... ",
             squash_timer_get_elapsed_cpu (context->compress_timer),
             squash_timer_get_elapsed_wall (context->compress_timer));

    squash_timer_start (context->decompress_timer);
    squash_codec_decompress_file_with_options (codec, decompressed, compressed, NULL);
    squash_timer_stop (context->decompress_timer);
    fprintf (stderr, "decompressed (%.6f CPU, %.6f wall).\n",
             squash_timer_get_elapsed_cpu (context->decompress_timer),
             squash_timer_get_elapsed_wall (context->decompress_timer));

    benchmark_report_result (codec, context, compressed, decompressed, -1);
    num_results++;

    squash_timer_reset (context->compress_timer);
    squash_timer_reset (context->decompress_timer);
    fclose (compressed);
    fclose (decompressed);
    fseek (context->input, 0, SEEK_SET);
  }

  squash_json_writer_end_container (context->json);
}

static void
benchmark_plugin (SquashPlugin* plugin, void* data) {
  struct BenchmarkContext* context = (struct BenchmarkContext*) data;

  /* Since we're often running against the source dir, we will pick up
     plugins which have not been compiled.  This should bail us out
     before trying to actually use them. */
  if (squash_plugin_init (plugin) != SQUASH_OK) {
    return;
  }

  squash_json_writer_begin_element_string_map (context->json, squash_plugin_get_name (plugin));
  squash_plugin_foreach_codec (plugin, benchmark_codec, data);
  squash_json_writer_end_container (context->json);
}

int main (int argc, char** argv) {
  struct BenchmarkContext context = { stdout, NULL, NULL, 0, BENCHMARK_OUTPUT_FORMAT_JSON, NULL, NULL };
  int opt;
  int optc = 0;
  SquashCodec* codec = NULL;

  context.compress_timer = squash_timer_new ();
  context.decompress_timer = squash_timer_new ();

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

  if (context.format == BENCHMARK_OUTPUT_FORMAT_JSON) {
    context.json = squash_json_writer_new (context.output);
  }

  benchmark_context_write_csv (&context, "Dataset,Plugin,Codec,Uncompressed Size,Compressed Size,Compression CPU Time,Compression Wall Clock Time,Decompression CPU Time,Decompression Wall Clock Time\n");

  while ( optind < argc ) {
    context.input_name = argv[optind];
    context.input = fopen (context.input_name, "r");
    if (context.input == NULL) {
      perror ("Unable to open input data");
      return -1;
    }

    if (fseek (context.input, 0, SEEK_END) != 0) {
      perror ("Unable to seek to end of input file");
      exit (-1);
    }
    context.input_size = ftell (context.input);

    fprintf (stderr, "Using %s:\n", context.input_name);

    squash_json_writer_begin_element_string_map (context.json, context.input_name);
    squash_json_writer_write_element_string_int (context.json, "uncompressed-size", (int) context.input_size);

    squash_json_writer_begin_element_string_map (context.json, "plugins");
    if (codec == NULL) {
      squash_foreach_plugin (benchmark_plugin, &context);
    } else {
      benchmark_codec (codec, &context);
    }
    squash_json_writer_end_container (context.json);

    squash_json_writer_end_container (context.json);

    optind++;
  }

  if (context.json != NULL) {
    squash_json_writer_free (context.json);
  }

  fclose (context.input);
  fclose (context.output);

  squash_timer_free (context.compress_timer);
  squash_timer_free (context.decompress_timer);

  return 0;
}
