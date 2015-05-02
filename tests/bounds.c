#include "test-codecs.h"

#include <unistd.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>

static size_t page_size = 0;

static void*
malloc_protected (void** protected_space, size_t size) {
  if (page_size == 0) {
    long ps = sysconf (_SC_PAGE_SIZE);
    page_size = (ps == -1) ? 8192 : ((size_t) ps);
  }

  size_t alloc_size =
    ((size / page_size) * page_size) +
    (((size % page_size) == 0) ? 0 : page_size) +
    (page_size * 2);

  uintptr_t ptr = (uintptr_t) malloc (alloc_size);
  uintptr_t pptr = ptr + alloc_size - page_size;
  pptr -= pptr % page_size;
  *protected_space = (void*) (pptr);

  if (mprotect (*protected_space, page_size, PROT_NONE) == -1)
    g_error ("mprotect failed: %s (%d)", strerror (errno), errno);

  return (void*) ptr;
}

void
check_codec (SquashCodec* codec) {
  uint8_t* uncompressed_start = NULL;
  uint8_t* uncompressed_protected = NULL;
  uint8_t* uncompressed = NULL;

  uint8_t* compressed_start;
  uint8_t* compressed_protected;
  uint8_t* compressed;

  uint8_t* decompressed_protected;
  uint8_t* decompressed;

  SquashStatus res;
  size_t tmp;
  size_t compressed_length;
  size_t decompressed_length;

  uncompressed_start = malloc_protected ((void**) &uncompressed_protected, LOREM_IPSUM_LENGTH);
  uncompressed = uncompressed_protected - LOREM_IPSUM_LENGTH;
  memcpy (uncompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH);

  compressed_start = malloc_protected ((void**) &compressed_protected, 8192);

  malloc_protected ((void**) &decompressed_protected, LOREM_IPSUM_LENGTH);

  compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  g_assert (compressed_length < 8192);

  /* Determine the true size of the compressed data. */
  compressed = compressed_start;
  res = squash_codec_compress_with_options (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, uncompressed, NULL);
  g_assert (res == SQUASH_OK);

  /* Decompress to a buffer which is exactly the right size */
  decompressed = decompressed_protected - LOREM_IPSUM_LENGTH;
  decompressed_length = LOREM_IPSUM_LENGTH;
  res = squash_codec_decompress_with_options (codec, &decompressed_length, decompressed, compressed_length, compressed, NULL);
  g_assert (res == SQUASH_OK);


  /* Compress to a buffer which is exactly the right size */
  compressed = compressed_protected - compressed_length;
  tmp = compressed_length;
  res = squash_codec_compress_with_options (codec, &tmp, compressed, LOREM_IPSUM_LENGTH, uncompressed, NULL);
  /* It's okay if some codecs require a few extra bytes to *compress*,
     as long as they don't write outside the buffer they were provided
     (which we're checking for with mprotect here. */

  /* Compress to a buffer which is too small */
  compressed = compressed_protected - (compressed_length - 1);
  tmp = compressed_length - 1;
  res = squash_codec_compress_with_options (codec, &tmp, compressed, LOREM_IPSUM_LENGTH, uncompressed, NULL);
  g_assert (res != SQUASH_OK);

  mprotect (uncompressed_protected, page_size, PROT_READ | PROT_WRITE);
  free (uncompressed_start);
  mprotect (compressed_protected, page_size, PROT_READ | PROT_WRITE);
  free (compressed_start);
}
