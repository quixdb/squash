#include <stdint.h>
#include <stdlib.h>

#include <squash/squash.h>

static SquashCodec* codec = NULL;
static bool codec_knows_decompressed_size = false;

HEDLEY_C_DECL
int
LLVMFuzzerInitialize(int *argc, char ***argv) {
  if (HEDLEY_UNLIKELY(*argc < 2)) {
    fprintf(stderr, "Usage: %s plugin-name libFuzzer_args...\n", (*argv)[0]);
    return EXIT_FAILURE;
  }

  codec = squash_get_codec((*argv)[1]);
  if (HEDLEY_UNLIKELY(codec == NULL)) {
    fprintf(stderr, "Unable to find codec `%s'\n", (*argv)[1]);
    return EXIT_FAILURE;
  }

  codec_knows_decompressed_size = (squash_codec_get_info (codec) & SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE) != 0;

  for (int pos = 2 ; pos < *argc ; pos++)
    (*argv)[pos - 1] = (*argv)[pos];
  *argc--;

  return EXIT_SUCCESS;
}

HEDLEY_C_DECL
int
LLVMFuzzerTestOneInput(const uint8_t *compressed, size_t compressed_size) {
  size_t decompressed_size;
  uint8_t* decompressed;

  if (codec_knows_decompressed_size) {
    decompressed_size = squash_codec_get_uncompressed_size (codec, compressed_size, compressed);
  } else {
    decompressed_size = compressed_size << 1;
  }

  decompressed = malloc (decompressed_size);
  if (HEDLEY_UNLIKELY(decompressed == NULL)) {
    return EXIT_FAILURE;
  }

  squash_codec_decompress (codec, &decompressed_size, decompressed, compressed_size, compressed, NULL);

  free (decompressed);

  return 0;
}
