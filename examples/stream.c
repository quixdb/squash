#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <squash/squash.h>

#define BUFFER_SIZE ((size_t) 1024 * 1024)

int main (int argc, char** argv) {
  int retval = EXIT_SUCCESS;

  if (argc != 3) {
    fprintf (stderr, "USAGE: %s (c|d) CODEC\n", argv[0]);
    fprintf (stderr, "Input is read from stdin, output is written to stdout\n");
    exit (-1);
  }

  SquashCodec* codec = squash_get_codec (argv[2]);
  if (codec == NULL) {
    fprintf (stderr, "Unable to find codec '%s'\n", argv[1]);
    return EXIT_FAILURE;
  }

  SquashStreamType stream_type;
  if (strcmp ("c", argv[1]) == 0)
    stream_type = SQUASH_STREAM_COMPRESS;
  else if (strcmp ("d", argv[1]) == 0)
    stream_type = SQUASH_STREAM_DECOMPRESS;
  else {
    fprintf (stderr, "Invalid mode '%s': must be 'c' or 'd'\n", argv[1]);
    return EXIT_FAILURE;
  }

  SquashStatus res;
  uint8_t* input = malloc (BUFFER_SIZE);
  size_t input_size;
  uint8_t* output = malloc (BUFFER_SIZE);
  SquashStream* stream = squash_stream_new_codec (codec, stream_type, NULL);

  if (stream == NULL || input == NULL || output == NULL) {
    fprintf (stderr, "Failed to allocate memory.\n");
    goto fail;
  }

  while (!feof (stdin)) {
    input_size = fread (input, 1, BUFFER_SIZE, stdin);
    if (!feof (stdin)) {
      fprintf (stderr, "Unable to read from input: %s\n", strerror (errno));
      goto fail;
    }

    stream->next_in = input;
    stream->avail_in = input_size;

    do {
      stream->next_out = output;
      stream->avail_out = BUFFER_SIZE;

      res = squash_stream_process (stream);

      if (res < 0) {
        fprintf (stderr, "Processing failed: %s (%d)\n", squash_status_to_string (res), res);
        goto fail;
      }

      const size_t output_size = BUFFER_SIZE - stream->avail_out;
      if (output_size != 0) {
        const size_t bytes_written = fwrite (output, 1, output_size, stdout);
        if (bytes_written != output_size) {
          fprintf (stderr, "Unable to write output: %s\n", strerror (errno));
          goto fail;
        }
      }
    } while (res == SQUASH_PROCESSING);
  }

  do {
    stream->next_out = output;
    stream->avail_out = BUFFER_SIZE;

    res = squash_stream_finish (stream);

    if (res < 0) {
      fprintf (stderr, "Finishing failed: %s (%d)\n", squash_status_to_string (res), res);
      goto fail;
    }

    const size_t output_size = BUFFER_SIZE - stream->avail_out;
    if (output_size != 0) {
      const size_t bytes_written = fwrite (output, 1, output_size, stdout);
      if (bytes_written != output_size) {
        fprintf (stderr, "Unable to write output: %s\n", strerror (errno));
        goto fail;
      }
    }
  } while (res == SQUASH_PROCESSING);

  goto cleanup;

 fail:
  retval = EXIT_FAILURE;

 cleanup:
  free (input);
  free (output);
  squash_object_unref (stream);

  return retval;
}
