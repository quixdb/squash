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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <squash/squash.h>

#include <pthread.h>

#include <blosc.h>

#define SQUASH_BLOSC_DEFAULT_LEVEL 6
#define SQUASH_BLOSC_DEFAULT_SHUFFLE 1
#define SQUASH_BLOSC_DEFAULT_TYPE_SIZE 1
#define SQUASH_BLOSC_DEFAULT_THREADS 1

static pthread_mutex_t squash_blosc_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct SquashBloscOptions_s {
  SquashOptions base_object;

  int level;
  int shuffle;
  int type_size;
  int threads;
} SquashBloscOptions;

SquashStatus               squash_plugin_init_codec     (SquashCodec* codec, SquashCodecFuncs* funcs);
SquashStatus               squash_plugin_init           (SquashPlugin* plugin);

static void                squash_blosc_options_init    (SquashBloscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify);
static SquashBloscOptions* squash_blosc_options_new     (SquashCodec* codec);
static void                squash_blosc_options_destroy (void* options);
static void                squash_blosc_options_free    (void* options);

static void
squash_blosc_options_init (SquashBloscOptions* options, SquashCodec* codec, SquashDestroyNotify destroy_notify) {
  assert (options != NULL);

  squash_options_init ((SquashOptions*) options, codec, destroy_notify);

  options->level = SQUASH_BLOSC_DEFAULT_LEVEL;
  options->shuffle = SQUASH_BLOSC_DEFAULT_SHUFFLE;
  options->type_size = SQUASH_BLOSC_DEFAULT_TYPE_SIZE;
}

static SquashBloscOptions*
squash_blosc_options_new (SquashCodec* codec) {
  SquashBloscOptions* options;

  options = (SquashBloscOptions*) malloc (sizeof (SquashBloscOptions));
  squash_blosc_options_init (options, codec, squash_blosc_options_free);

  return options;
}

static SquashOptions*
squash_blosc_create_options (SquashCodec* codec) {
  return (SquashOptions*) squash_blosc_options_new (codec);
}

static SquashStatus
squash_blosc_parse_option (SquashOptions* options, const char* key, const char* value) {
  SquashBloscOptions* opts = (SquashBloscOptions*) options;
  char* endptr = NULL;

  assert (opts != NULL);

  if (strcasecmp (key, "level") == 0) {
    const int level = (int) strtol (value, &endptr, 0);
    if ( *endptr == '\0' && level >= 0 && level <= 9 ) {
      opts->level = level;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "type-size") == 0) {
    const long type_size = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && type_size >= 0 && type_size <= INT_MAX ) {
      opts->type_size = (int) type_size;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "shuffle") == 0) {
    if (strcasecmp (value, "true") == 0) {
      opts->shuffle = true;
    } else if (strcasecmp (value, "false")) {
      opts->shuffle = false;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else if (strcasecmp (key, "threads") == 0) {
    const long threads = strtol (value, &endptr, 0);
    if ( *endptr == '\0' && threads >= 1 && threads <= BLOSC_MAX_THREADS ) {
      opts->threads = (int) threads;
    } else {
      return SQUASH_BAD_VALUE;
    }
  } else {
    return SQUASH_BAD_PARAM;
  }

  return SQUASH_OK;
}

static void
squash_blosc_options_destroy (void* options) {
  squash_options_destroy ((SquashOptions*) options);
}

static void
squash_blosc_options_free (void* options) {
  squash_blosc_options_destroy ((SquashBloscOptions*) options);
  free (options);
}

static size_t
squash_blosc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_length) {
  return uncompressed_length + BLOSC_MAX_OVERHEAD;
}

static size_t
squash_blosc_get_uncompressed_size (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length) {
  size_t nbytes, cbytes, blocksize;

  if (compressed_length < BLOSC_MIN_HEADER_LENGTH) {
    return SQUASH_FAILED;
  }

  blosc_cbuffer_sizes (compressed, &nbytes, &cbytes, &blocksize);
  if (compressed_length < cbytes) {
    return 0;
  }

  return nbytes;
}

static SquashStatus
squash_blosc_compress_buffer (SquashCodec* codec,
                               uint8_t* compressed, size_t* compressed_length,
                               const uint8_t* uncompressed, size_t uncompressed_length,
                               SquashOptions* options) {
  SquashBloscOptions* opts = (SquashBloscOptions*) options;

  pthread_mutex_lock (&squash_blosc_lock);
  blosc_set_nthreads ((opts != NULL) ? opts->threads : SQUASH_BLOSC_DEFAULT_THREADS);
  int e = blosc_compress ((opts != NULL) ? opts->level : SQUASH_BLOSC_DEFAULT_LEVEL,
                          (opts != NULL) ? opts->shuffle : SQUASH_BLOSC_DEFAULT_SHUFFLE,
                          (opts != NULL) ? opts->type_size : SQUASH_BLOSC_DEFAULT_TYPE_SIZE,
                          uncompressed_length,
                          uncompressed,
                          compressed,
                          *compressed_length);
  pthread_mutex_unlock (&squash_blosc_lock);

  if (e > 0) {
    *compressed_length = e;
    return SQUASH_OK;
  } else if (e < 0) {
    return SQUASH_FAILED;
  } else {
    return SQUASH_BUFFER_FULL;
  }
}

static SquashStatus
squash_blosc_decompress_buffer (SquashCodec* codec,
                                uint8_t* decompressed, size_t* decompressed_length,
                                const uint8_t* compressed, size_t compressed_length,
                                SquashOptions* options) {
  int e;
  size_t nbytes, cbytes, blocksize;

  if (compressed_length < BLOSC_MIN_HEADER_LENGTH) {
    return SQUASH_FAILED;
  }

  blosc_cbuffer_sizes (compressed, &nbytes, &cbytes, &blocksize);
  if (compressed_length < cbytes) {
    return SQUASH_BUFFER_EMPTY;
  } else if (*decompressed_length < nbytes) {
    return SQUASH_BUFFER_FULL;
  }

  pthread_mutex_lock (&squash_blosc_lock);
  blosc_set_nthreads ((options != NULL) ? ((SquashBloscOptions*) options)->threads : SQUASH_BLOSC_DEFAULT_THREADS);
  e = blosc_decompress (compressed, decompressed, *decompressed_length);
  pthread_mutex_unlock (&squash_blosc_lock);

  if (e > 0) {
    *decompressed_length = e;
    return SQUASH_OK;
  } else if (e < 0) {
    return SQUASH_FAILED;
  } else {
    return SQUASH_BUFFER_FULL;
  }
}

SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecFuncs* funcs) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp ("blosc", name) == 0) {
    funcs->create_options = squash_blosc_create_options;
    funcs->parse_option = squash_blosc_parse_option;
    funcs->get_uncompressed_size = squash_blosc_get_uncompressed_size;
    funcs->get_max_compressed_size = squash_blosc_get_max_compressed_size;
    funcs->decompress_buffer = squash_blosc_decompress_buffer;
    funcs->compress_buffer = squash_blosc_compress_buffer;
  } else {
    return SQUASH_UNABLE_TO_LOAD;
  }

  return SQUASH_OK;
}

SquashStatus squash_plugin_init (SquashPlugin* plugin) {
  blosc_init ();

  return SQUASH_OK;
}
