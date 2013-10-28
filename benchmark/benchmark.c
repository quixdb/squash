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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <squash/squash.h>
#include "timer.h"
#include "json-writer.h"

#define SQUASH_BENCHMARK_MIN_EXEC_TIME 5.0

#if defined(__MINGW32__)
#define squash_tmpfile() tmpfile()
#elif !defined(_WIN32)
static FILE*
squash_tmpfile () {
  char template[] = "squash-benchmark-XXXXXX";
  int fd = mkstemp (template);
  FILE* res = NULL;

  if (fd != -1) {
    unlink (template);
    res = fdopen (fd, "w+b");
  }

  return res;
}
#else
static FILE*
squash_tmpfile () {
  FILE* res = NULL;
  return tmpfile_s (&res) == 0 ? res : NULL;
}
#endif

static void
print_help_and_exit (int argc, char** argv, int exit_code) {
  fprintf (stderr, "Usage: %s [OPTION]... FILE...\n", argv[0]);
  fprintf (stderr, "Benchmark Squash plugins.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-h            Print this help screen and exit.\n");
  fprintf (stderr, "\t-c codec      Benchmark the specified codec and exit.\n");
  fprintf (stderr, "\t-j outfile    JSON output file.\n");
  fprintf (stderr, "\t-s outfile    CSV output file.\n");

  exit (exit_code);
}

struct BenchmarkContext {
  FILE* csv;
  FILE* input;
  char* input_name;
  long input_size;
  SquashJSONWriter* json;
};

typedef struct {
  long int compressed_size;
  double compress_cpu;
  double compress_wall;
  double decompress_cpu;
  double decompress_wall;
} SquashBenchmarkResult;

static bool
benchmark_codec_with_options (struct BenchmarkContext* context, SquashCodec* codec, SquashOptions* opts, int level) {
  SquashBenchmarkResult result = { 0, 0.0, 0.0, 0.0, 0.0 };
  bool success = false;
  SquashStatus res = SQUASH_OK;

#if !defined(_WIN32)
  char fifo_name[] = ".squash-benchmark-fifo-XXXXXX";
  int descriptor;

  assert (mkfifo (mktemp (fifo_name), 0600) == 0);

  if (fork () == 0) {
    descriptor = open (fifo_name, O_WRONLY);
#else
  {
#endif
    FILE* compressed = squash_tmpfile ();
    FILE* decompressed = squash_tmpfile ();
    SquashTimer* timer = squash_timer_new ();
    int iterations = 0;

    if (level < 0) {
      fputs ("    compressing: ", stderr);
    } else {
      fprintf (stderr, "    level %d: ", level);
    }

    if (fseek (context->input, 0, SEEK_SET) != 0) {
      perror ("Unable to seek to beginning of input file");
      exit (-1);
    }

    for ( iterations = 0 ; squash_timer_get_elapsed_cpu (timer) < SQUASH_BENCHMARK_MIN_EXEC_TIME ; iterations++ ) {
      fseek (context->input, 0, SEEK_SET);
      fseek (compressed, 0, SEEK_SET);

      squash_timer_start (timer);
      res = squash_codec_compress_file_with_options (codec, compressed, context->input, opts);
      squash_timer_stop (timer);

      if (res != SQUASH_OK) {
        break;
      }
    }

    if (res == SQUASH_OK) {
      result.compressed_size = ftell (compressed);
      result.compress_cpu = squash_timer_get_elapsed_cpu (timer) / iterations;
      result.compress_wall = squash_timer_get_elapsed_wall (timer) / iterations;
      squash_timer_reset (timer);

      if (result.compressed_size == 0) {
        fprintf (stderr, "error (%s)... ", squash_status_to_string (res));
      } else {
        fprintf (stderr, "compressed (%.4f CPU, %.4f wall, %ld bytes)... ",
                 result.compress_cpu,
                 result.compress_wall,
                 result.compressed_size);

        for ( iterations = 0 ; squash_timer_get_elapsed_cpu (timer) < SQUASH_BENCHMARK_MIN_EXEC_TIME ; iterations++ ) {
          fseek (compressed, 0, SEEK_SET);
          fseek (decompressed, 0, SEEK_SET);

          squash_timer_start (timer);
          res = squash_codec_decompress_file_with_options (codec, decompressed, compressed, opts);
          squash_timer_stop (timer);

          if (res != SQUASH_OK) {
            break;
          }
        }

        if (res != SQUASH_OK) {
          fprintf (stderr, "error (%s)... ", squash_status_to_string (res));
        } else {
          result.decompress_cpu = squash_timer_get_elapsed_cpu (timer) / iterations;
          result.decompress_wall = squash_timer_get_elapsed_wall (timer) / iterations;
          squash_timer_reset (timer);

          if (ftell (decompressed) != context->input_size) {
            /* Should never happen. */
            fputs ("Size mismatch.\n", stderr);
          } else {
            fprintf (stderr, "decompressed (%.6f CPU, %.6f wall).\n",
                     result.decompress_cpu,
                     result.decompress_wall);

#if !defined(_WIN32)
            write (descriptor, &result, sizeof (SquashBenchmarkResult));
#endif
          }
        }
      }
    }

    fflush (stderr);

    squash_timer_free (timer);
    fclose (compressed);
    fclose (decompressed);

#if !defined(_WIN32)
    close (descriptor);
    exit (0);
  } else {
    descriptor = open (fifo_name, O_RDONLY);
    size_t bytes_read = read (descriptor, &result, sizeof (SquashBenchmarkResult));
    wait (NULL);
    if (bytes_read != sizeof (SquashBenchmarkResult)) {
#else
  } {
    if (res != SQUASH_OK) {
#endif
      fputs ("Failed.\n", stderr);
    } else {
      if (context->json != NULL) {
        squash_json_writer_begin_value_map (context->json);

        if (level >= 0)
          squash_json_writer_write_element_string_int (context->json, "level", level);
        squash_json_writer_write_element_string_int (context->json, "compressed-size", result.compressed_size);
        squash_json_writer_write_element_string_double (context->json, "compress-cpu", result.compress_cpu);
        squash_json_writer_write_element_string_double (context->json, "compress-wall", result.compress_wall);
        squash_json_writer_write_element_string_double (context->json, "decompress-cpu", result.decompress_cpu);
        squash_json_writer_write_element_string_double (context->json, "decompress-wall", result.decompress_wall);
        squash_json_writer_end_container (context->json);
      }

      if (context->csv != NULL) {
        if (level >= 0) {
          fprintf (context->csv, "%s,%s,%s,%d,%ld,%ld,%f,%f,%f,%f\n",
                   context->input_name,
                   squash_plugin_get_name (squash_codec_get_plugin (codec)),
                   squash_codec_get_name (codec),
                   level,
                   context->input_size,
                   result.compressed_size,
                   result.compress_cpu,
                   result.compress_wall,
                   result.decompress_cpu,
                   result.decompress_wall);
        } else {
          fprintf (context->csv, "%s,%s,%s,,%ld,%ld,%f,%f,%f,%f\n",
                   context->input_name,
                   squash_plugin_get_name (squash_codec_get_plugin (codec)),
                   squash_codec_get_name (codec),
                   context->input_size,
                   result.compressed_size,
                   result.compress_cpu,
                   result.compress_wall,
                   result.decompress_cpu,
                   result.decompress_wall);
        }
      }

      success = true;
    }
#if !defined(_WIN32)
    unlink (fifo_name);
    close (descriptor);
#endif
  }

  return success;
}

static void
benchmark_codec (SquashCodec* codec, void* data) {
  struct BenchmarkContext* context = (struct BenchmarkContext*) data;
  SquashOptions* opts;
  int level = 0;
  char level_s[4];
  SquashStatus res = SQUASH_OK;
  bool have_results = false;

  umask (0100);

  fprintf (stderr, "%s:%s\n",
           squash_plugin_get_name (squash_codec_get_plugin (codec)),
           squash_codec_get_name (codec));

  if (context->json != NULL) {
    squash_json_writer_begin_element_string_array (context->json, squash_codec_get_name (codec));
  }

  opts = squash_options_new (codec, NULL);
  if (opts != NULL) {
    squash_object_ref_sink (opts);
    for ( level = 0 ; level <= 999 ; level++ ) {
      snprintf (level_s, 4, "%d", level);
      if (squash_options_parse_option (opts, "level", level_s) == SQUASH_OK) {
        if (benchmark_codec_with_options (context, codec, opts, level)) {
          have_results = true;
        }
      }
    }
    squash_object_unref (opts);
  }

  if (!have_results) {
    benchmark_codec_with_options (context, codec, NULL, -1);
  }

  if (context->json != NULL) {
    squash_json_writer_end_container (context->json);
  }
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

  if (context->json != NULL) {
    squash_json_writer_begin_element_string_map (context->json, squash_plugin_get_name (plugin));
  }

  squash_plugin_foreach_codec (plugin, benchmark_codec, data);

  if (context->json != NULL) {
    squash_json_writer_end_container (context->json);
  }
}

int main (int argc, char** argv) {
  struct BenchmarkContext context = { NULL, NULL, NULL, 0 };
  int opt;
  int optc = 0;
  SquashCodec* codec = NULL;
  FILE* json_output = NULL;

  while ( (opt = getopt(argc, argv, "hc:j:s:")) != -1 ) {
    switch ( opt ) {
      case 'h':
        print_help_and_exit (argc, argv, 0);
        break;
      case 'j':
        json_output = fopen (optarg, "w+");
        if (json_output == NULL) {
          perror ("Unable to open output file");
          return -1;
        }
        setbuf (json_output, NULL);
        context.json = squash_json_writer_new (json_output);
        break;
      case 's':
        context.csv = fopen (optarg, "w+");
        if (context.csv == NULL) {
          perror ("Unable to open output file");
          return -1;
        }
        setbuf (context.csv, NULL);
        break;
      case 'c':
        codec = squash_get_codec ((const char*) optarg);
        if (codec == NULL) {
          fprintf (stderr, "Unable to find codec.\n");
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

  if (context.csv != NULL) {
    fprintf (context.csv, "Dataset,Plugin,Codec,Level,Uncompressed Size,Compressed Size,Compression CPU Time,Compression Wall Clock Time,Decompression CPU Time,Decompression Wall Clock Time\n");
  }

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

    if (context.json != NULL) {
      squash_json_writer_begin_element_string_map (context.json, context.input_name);
      squash_json_writer_write_element_string_int (context.json, "uncompressed-size", (int) context.input_size);
      squash_json_writer_begin_element_string_map (context.json, "plugins");
    }

    if (codec == NULL) {
      squash_foreach_plugin (benchmark_plugin, &context);
    } else {
      benchmark_codec (codec, &context);
    }

    if (context.json != NULL) {
      squash_json_writer_end_container (context.json);
      squash_json_writer_end_container (context.json);
    }

    optind++;
  }

  if (context.json != NULL) {
    squash_json_writer_free (context.json);
  }
  if (context.csv != NULL) {
    fclose (context.csv);
  }
  fclose (context.input);

  return 0;
}
