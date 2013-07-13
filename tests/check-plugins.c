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

/*********************************************************************
 *     ATTENTION * ATTENTION * ATTENTION * ATTENTION * ATTENTION     *
 *********************************************************************
 * Do NOT use test cases as examples.  They are often intentionally  *
 * bad and try break things, and they are almost always very         *
 * inelegant.  In other words, more often than not they are examples *
 * of what *not* to do.                                              *
 *                                                                   *
 * There is an examples/ folder where you can find good examples, or *
 * you can look in the documentation (which is available on our web  *
 * site).                                                            *
 ********************************************************************/

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <squash/squash.h>

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define SQUASH_ASSERT_OK(expr)                                          \
  do {                                                                  \
    if ((expr) != SQUASH_OK) {                                          \
      fprintf (stderr, "%s:%d: %s (%d)\n",__FILE__, __LINE__, squash_status_to_string ((expr)), (expr)); \
      exit (SIGTRAP);                                                   \
    }                                                                   \
  } while (0)

#define SQUASH_ASSERT_NO_ERROR(expr)                                    \
  do {                                                                  \
    if ((expr) < 0) {                                                   \
      fprintf (stderr, "%s:%d: %s (%d)\n",__FILE__, __LINE__, squash_status_to_string ((expr)), (expr)); \
      exit (SIGTRAP);                                                   \
    }                                                                   \
  } while (0)


const size_t LOREM_IPSUM_LENGTH = 2725;
#define LOREM_IPSUM                                                     \
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed vulputate " \
  "lectus nisl, vitae ultricies justo dictum nec. Vestibulum ante ipsum " \
  "primis in faucibus orci luctus et ultrices posuere cubilia Curae; "  \
  "Suspendisse suscipit quam a lectus adipiscing, sed tempor purus "    \
  "cursus. Vivamus id nulla eget elit eleifend molestie. Integer "      \
  "sollicitudin lorem enim, eu eleifend orci facilisis sed. Pellentesque " \
  "sodales luctus enim vel viverra. Cras interdum vel nisl in "         \
  "facilisis. Curabitur sollicitudin tortor vel congue "                \
  "auctor. Suspendisse egestas orci vitae neque placerat blandit.\n"    \
  "\n"                                                                  \
  "Aenean sed nisl ultricies, vulputate lorem a, suscipit nulla. Donec " \
  "egestas volutpat neque a eleifend. Nullam porta semper "             \
  "nunc. Pellentesque adipiscing molestie magna, quis pulvinar metus "  \
  "gravida sit amet. Vestibulum mollis et sapien eu posuere. Quisque "  \
  "tristique dignissim ante et aliquet. Phasellus vulputate condimentum " \
  "nulla in vulputate.\n"                                               \
  "\n"                                                                  \
  "Nullam volutpat tellus at nisi auctor, vitae mattis nibh viverra. Nunc " \
  "vitae lectus tristique, ultrices nibh quis, lobortis elit. Curabitur " \
  "at vestibulum nisi, nec facilisis ante. Nulla pharetra blandit lacus, " \
  "at sodales nulla placerat eget. Nulla congue varius tortor, sit amet " \
  "tempor est mattis nec. Praesent vitae tristique ipsum, rhoncus "     \
  "tristique lorem. Sed et erat tristique ligula accumsan fringilla eu in " \
  "urna. Donec dapibus hendrerit neque nec venenatis. In euismod sapien " \
  "ipsum, auctor consectetur mi dapibus hendrerit.\n"                   \
  "\n"                                                                  \
  "Phasellus sagittis rutrum velit, in sodales nibh imperdiet a. Integer " \
  "vitae arcu blandit nibh laoreet scelerisque eu sit amet eros. Aenean " \
  "odio felis, aliquam in eros at, ornare luctus magna. In semper "     \
  "tincidunt nunc, sollicitudin gravida nunc laoreet eu. Cras eu tempor " \
  "sapien, ut dignissim elit. Proin eleifend arcu tempus, semper erat et, " \
  "accumsan erat. Praesent vulputate diam mi, eget mollis leo "         \
  "pellentesque eget. Aliquam eu tortor posuere, posuere velit sed, "   \
  "suscipit eros. Nam eu leo vitae mauris condimentum lobortis non quis " \
  "mauris. Nulla venenatis fringilla urna nec venenatis. Nam eget velit " \
  "nulla. Proin ut malesuada felis. Suspendisse vitae nunc neque. Donec " \
  "faucibus tempor lacinia. Vivamus ac vulputate sapien, eget lacinia " \
  "nisl.\n"                                                             \
  "\n"                                                                  \
  "Curabitur eu dolor molestie, ullamcorper lorem quis, egestas "       \
  "urna. Suspendisse in arcu sed justo blandit condimentum. Ut auctor, " \
  "sem quis condimentum mattis, est purus pulvinar elit, quis viverra " \
  "nibh metus ac diam. Etiam aliquet est eu dui fermentum consequat. Cras " \
  "auctor diam eget bibendum sagittis. Aenean elementum purus sit amet " \
  "sem euismod, non varius felis dictum. Aliquam tempus pharetra ante a " \
  "sagittis. Curabitur ut urna felis. Etiam sed vulputate nisi. Praesent " \
  "at libero eleifend, sagittis quam a, varius sapien."

static void
check_buffer_basic (gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t* uncompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  res = squash_codec_compress_with_options (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH, NULL);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, uncompressed, &uncompressed_length, compressed, compressed_length, NULL);
  SQUASH_ASSERT_OK(res);
  if (uncompressed_length != LOREM_IPSUM_LENGTH)
    fprintf (stderr, "Uncompressed length: %lu (expected %lu)\n", uncompressed_length, LOREM_IPSUM_LENGTH);
  // g_assert (uncompressed_length == LOREM_IPSUM_LENGTH);

  g_assert (memcmp (LOREM_IPSUM, uncompressed, LOREM_IPSUM_LENGTH) == 0);

  free (compressed);
  free (uncompressed);
}

static SquashStatus
buffer_to_buffer_compress_with_stream (SquashCodec* codec,
                                       uint8_t* compressed, size_t* compressed_length,
                                       uint8_t* uncompressed, size_t uncompressed_length) {
  size_t step_size = g_test_rand_int_range (1, 255);
  SquashStream* stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_COMPRESS, NULL);
  SquashStatus res;

	stream->next_out = compressed;
	stream->avail_out = (step_size < *compressed_length) ? step_size : *compressed_length;
  stream->next_in = uncompressed;

  while (stream->total_in < uncompressed_length) {
    stream->avail_in = MIN(uncompressed_length - stream->total_in, step_size);

    do {
      res = squash_stream_process (stream);

      if (stream->avail_out < step_size) {
        stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);
      }
    } while (res == SQUASH_PROCESSING);

    if (res != SQUASH_OK) {
      fprintf (stderr, "!!!!!!!!!!! %d\n", res);
      break;
    }
    SQUASH_ASSERT_OK(res);
  }

  do {
    stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);

    res = squash_stream_finish (stream);
	} while (res == SQUASH_PROCESSING);

  if (res == SQUASH_OK) {
    *compressed_length = stream->total_out;
  }

  squash_object_unref (stream);

  return res;
}

static void
check_stream_compress (gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t uncompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  uint8_t* uncompressed = (uint8_t*) malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;

  res = buffer_to_buffer_compress_with_stream (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH);
  SQUASH_ASSERT_OK(res);

  res = squash_codec_decompress_with_options (codec, uncompressed, &uncompressed_length, compressed, compressed_length, NULL);
  SQUASH_ASSERT_OK(res);
  g_assert (uncompressed_length == LOREM_IPSUM_LENGTH);

  g_assert (memcmp (LOREM_IPSUM, uncompressed, uncompressed_length) == 0);

  free (compressed);
  free (uncompressed);
}

static SquashStatus
buffer_to_buffer_decompress_with_stream (SquashCodec* codec,
                                         uint8_t* decompressed, size_t* decompressed_length,
                                         uint8_t* compressed, size_t compressed_length) {
  size_t step_size = g_test_rand_int_range (1, 255);
  SquashStream* stream = squash_codec_create_stream_with_options (codec, SQUASH_STREAM_DECOMPRESS, NULL);
  SquashStatus res = SQUASH_OK;

	stream->next_out = decompressed;
	stream->avail_out = (step_size < *decompressed_length) ? step_size : *decompressed_length;
  stream->next_in = compressed;

  while (stream->total_in < compressed_length &&
         stream->total_out < *decompressed_length) {
    stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
    stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

    res = squash_stream_process (stream);
    if (res == SQUASH_END_OF_STREAM || res < 0) {
      break;
    }
  }

  if (res > 0) {
    do {
      stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
      stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

      res = squash_stream_finish (stream);
    } while (res != SQUASH_OK);
  }

  squash_object_unref (stream);

  return (res > 0) ? SQUASH_OK : res;
}

static void
check_stream_decompress (gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  uint8_t* decompressed;
  size_t decompressed_length;
  size_t compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  uint8_t* compressed = (uint8_t*) malloc (compressed_length);
  SquashStatus res;

  res = squash_codec_compress_with_options (codec, compressed, &compressed_length, (uint8_t*) LOREM_IPSUM, LOREM_IPSUM_LENGTH, NULL);
  SQUASH_ASSERT_OK(res);

  if (squash_codec_get_knows_uncompressed_size (codec)) {
    decompressed_length = squash_codec_get_uncompressed_size (codec, compressed, compressed_length);
    g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  } else {
    decompressed_length = LOREM_IPSUM_LENGTH;
  }
  decompressed = (uint8_t*) malloc (decompressed_length);

  res = buffer_to_buffer_decompress_with_stream (codec, decompressed, &decompressed_length, compressed, compressed_length);
  SQUASH_ASSERT_OK(res);

  g_assert (decompressed_length == LOREM_IPSUM_LENGTH);
  g_assert (memcmp (LOREM_IPSUM, decompressed, decompressed_length) == 0);

  free (compressed);
  free (decompressed);
}

static void squash_check_codec_enumerator_codec_cb (SquashCodec* codec, void* data) {
  gchar* test_name = NULL;

  if (squash_codec_init (codec) != SQUASH_OK) {
    /* If srcdir == builddir then we will pick up *all* of the
       plugins, not just those which were enabled.  This should filter
       out any disabled plugins. */
    return;
  }

  test_name = g_strdup_printf ("/plugins/%s/buffer/basic", squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, check_buffer_basic);
  g_free (test_name);

  test_name = g_strdup_printf ("/plugins/%s/stream/compress", squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, check_stream_compress);
  g_free (test_name);

  test_name = g_strdup_printf ("/plugins/%s/stream/decompress", squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, check_stream_decompress);
  g_free (test_name);
}

static void squash_check_codec_enumerator_plugin_cb (SquashPlugin* plugin, void* data) {
  squash_plugin_foreach_codec (plugin, squash_check_codec_enumerator_codec_cb, data);
}

int
main (int argc, char** argv) {
  g_test_init (&argc, &argv, NULL);

  squash_foreach_plugin (squash_check_codec_enumerator_plugin_cb, NULL);

  return g_test_run ();
}
