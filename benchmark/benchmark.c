#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <squash/squash.h>

static void
print_help_and_exit (int argc, char** argv, int exit_code) {
  fprintf (stderr, "Usage: %s [OPTION]... FILE...\n", argv[0]);
  fprintf (stderr, "Benchmark Squash plugins.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-o outfile    Write data to outfile (default is stdout)\n");
  fprintf (stderr, "\t-h            Print this help screen and exit.\n");

  exit (exit_code);
}

struct BenchmarkContext {
  FILE* output;
  FILE* input;
  char* input_name;
  bool first;
};

struct BenchmarkTimer {
  struct timespec start_wall;
  struct timespec end_wall;
  struct timespec start_cpu;
  struct timespec end_cpu;
};

static void
benchmark_timer_start (struct BenchmarkTimer* timer) {
  if (clock_gettime (CLOCK_REALTIME, &(timer->start_wall)) != 0) {
    perror ("Unable to get wall clock time");
    exit (errno);
  }
  if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &(timer->start_cpu)) != 0) {
    perror ("Unable to get CPU clock time");
    exit (errno);
  }
}

static void
benchmark_timer_stop (struct BenchmarkTimer* timer) {
  if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &(timer->end_cpu)) != 0) {
    perror ("Unable to get CPU clock time");
    exit (errno);
  }
  if (clock_gettime (CLOCK_REALTIME, &(timer->end_wall)) != 0) {
    perror ("Unable to get wall clock time");
    exit (errno);
  }
}

static double
benchmark_timer_elapsed (struct timespec* start, struct timespec* end) {
  return
    (double) (end->tv_sec - start->tv_sec) +
    (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000);
}

static double
benchmark_timer_elapsed_cpu (struct BenchmarkTimer* timer) {
  return benchmark_timer_elapsed (&(timer->start_cpu), &(timer->end_cpu));
}

static double
benchmark_timer_elapsed_wall (struct BenchmarkTimer* timer) {
  return benchmark_timer_elapsed (&(timer->start_wall), &(timer->end_wall));
}

static void
benchmark_codec (SquashCodec* codec, void* data) {
  struct BenchmarkContext* context = (struct BenchmarkContext*) data;
  FILE* compressed = tmpfile ();
  FILE* decompressed = tmpfile ();
  struct BenchmarkTimer timer;

  fprintf (stderr, "\t%s:%s\n",
           squash_plugin_get_name (squash_codec_get_plugin (codec)),
           squash_codec_get_name (codec));

  if (fseek (context->input, 0, SEEK_SET) != 0) {
    perror ("Unable to seek to beginning of input file");
    exit (-1);
  }

  if (context->first) {
    context->first = false;
  } else {
    fputc (',', context->output);
  }

  fputs ("\t\tcompressing... ", stderr);
  benchmark_timer_start (&timer);
  squash_codec_compress_file_with_options (codec, compressed, context->input, NULL);
  benchmark_timer_stop (&timer);
  fprintf (context->output, "{\"plugin\":\"%s\",\"codec\":\"%s\",\"size\":%ld,\"compress_cpu\":%g,\"compress_wall\":%g,",
           squash_plugin_get_name (squash_codec_get_plugin (codec)),
           squash_codec_get_name (codec),
           ftell (compressed),
           benchmark_timer_elapsed_cpu (&timer),
           benchmark_timer_elapsed_wall (&timer));
  fputs ("done.\n", stderr);

  if (fseek (compressed, 0, SEEK_SET) != 0) {
    perror ("Unable to seek to beginning of compressed file");
    exit (-1);
  }

  fputs ("\t\tdecompressing... ", stderr);
  benchmark_timer_start (&timer);
  squash_codec_decompress_file_with_options (codec, decompressed, compressed, NULL);
  benchmark_timer_stop (&timer);
  fprintf (context->output, "\"decompress_cpu\":%g,\"decompress_wall\":%g}",
           benchmark_timer_elapsed_cpu (&timer),
           benchmark_timer_elapsed_wall (&timer));
  fputs ("done.\n", stderr);

  fclose (compressed);
  fclose (decompressed);
}

static void
benchmark_plugin (SquashPlugin* plugin, void* data) {
  squash_plugin_foreach_codec (plugin, benchmark_codec, data);
}

int main (int argc, char** argv) {
  struct BenchmarkContext context = { stdout, NULL, NULL, true };
  bool first_input = true;
  long input_size = 0;
  int opt;
  int optc;

  while ( (opt = getopt(argc, argv, "ho:")) != -1 ) {
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
    }

    optc++;
  }

  if ( optind >= argc ) {
    fputs ("No input files specified.\n", stderr);
    return -1;
  }

  fputc ('{', context.output);

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
    input_size = ftell (context.input);

    fprintf (stderr, "Using %s:\n", context.input_name);
    if (first_input) {
      first_input = false;
    } else {
      fputc (',', context.output);
    }
    fprintf (context.output, "\"%s\":{\"uncompressed-size\":%ld,\"data\":[", context.input_name, input_size);

    squash_foreach_plugin (benchmark_plugin, &context);

    fputs ("]}", context.output);

    optind++;
  }

  fputs ("}\n", context.output);

  return 0;
}
