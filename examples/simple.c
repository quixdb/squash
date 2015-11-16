#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <squash/squash.h>

int main (int argc, char** argv) {
  SquashCodec* codec;
  const char* uncompressed;
  size_t uncompressed_length;
  size_t compressed_length;
  uint8_t* compressed;
  size_t decompressed_length;
  char* decompressed;

  if (argc != 3) {
    fprintf (stderr, "USAGE: %s ALGORITHM STRING\n", argv[0]);
    return EXIT_FAILURE;
  }

  codec = squash_get_codec (argv[1]);
  if (codec == NULL) {
    fprintf (stderr, "Unable to find algorithm '%s'.\n", argv[1]);
    return EXIT_FAILURE;
  }

  uncompressed = argv[2];
  uncompressed_length = strlen (uncompressed);
  compressed_length = squash_codec_get_max_compressed_size (codec, uncompressed_length);
  compressed = (uint8_t*) malloc (compressed_length);
  decompressed_length = uncompressed_length + 1;
  decompressed = (char*) malloc (uncompressed_length + 1);

  SquashStatus res =
    squash_codec_compress (codec,
                           &compressed_length, compressed,
                           uncompressed_length, (const uint8_t*) uncompressed,
                           NULL);
  if (res != SQUASH_OK) {
    fprintf (stderr, "Unable to compress data [%d]: %s\n",
             res, squash_status_to_string (res));
    return EXIT_FAILURE;
  }

  fprintf (stdout, "Compressed a %u byte buffer to %u bytes.\n",
           (unsigned int) uncompressed_length, (unsigned int) compressed_length);

  res = squash_codec_decompress (codec,
                                 &decompressed_length, (uint8_t*) decompressed,
                                 compressed_length, compressed, NULL);

  if (res != SQUASH_OK) {
    fprintf (stderr, "Unable to decompress data [%d]: %s\n",
             res, squash_status_to_string (res));
    return EXIT_FAILURE;
  }

  /* Notice that we didn't compress the *NULL* byte at the end of the
     string.  We could have, it's just a waste to do so. */
  decompressed[decompressed_length] = '\0';

  if (strcmp (decompressed, uncompressed) != 0) {
    fprintf (stderr, "Bad decompressed data.\n");
    EXIT_FAILURE;
  }

  fprintf (stdout, "Successfully decompressed.\n");

  return EXIT_SUCCESS;
}
