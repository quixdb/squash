#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_26
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_26

#include <glib.h>
#include <squash/squash.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define STREAM_TEST_CODEC "zlib:gzip"
#define BUFFER_TEST_CODEC "snappy:snappy"

#define LOREM_IPSUM (const uint8_t*)                                    \
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
#define LOREM_IPSUM_LENGTH ((size_t) 2725)

struct Single {
  char* filename;
  int fd;
};

static void
single_setup (struct Single* data, gconstpointer user_data) {
  GError* e = NULL;

  data->fd = g_file_open_tmp ("squash-file-test-XXXXXX", &(data->filename), &e);
  assert (e == NULL);
}

static void
single_teardown (struct Single* data, gconstpointer user_data) {
  unlink (data->filename);
  close (data->fd);
}

static void
test_file_io (struct Single* data, gconstpointer user_data) {
  SquashFile* file = squash_file_open (data->filename, "w+", STREAM_TEST_CODEC, NULL);
  g_assert (file != NULL);

  SquashStatus res = squash_file_write (file, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  g_assert (res == SQUASH_OK);

  squash_file_close (file);

  file = squash_file_open (data->filename, "rb", STREAM_TEST_CODEC, NULL);
  g_assert (file != NULL);

  uint8_t decompressed[LOREM_IPSUM_LENGTH];
  size_t total_read = 0;
  do {
    size_t bytes_read = 256;
    res = squash_file_read (file, &bytes_read, decompressed + total_read);
    assert (res > 0);
    total_read += bytes_read;
    assert (total_read <= LOREM_IPSUM_LENGTH);
  } while (!squash_file_eof (file));

  g_assert_cmpint (total_read, ==, LOREM_IPSUM_LENGTH);
  g_assert (memcmp (decompressed, LOREM_IPSUM, LOREM_IPSUM_LENGTH) == 0);

  squash_file_close (file);
}

struct Triple {
  char* filename[3];
  int fd[3];
};

static void
triple_setup (struct Triple* data, gconstpointer user_data) {
  GError* e = NULL;

  data->fd[0] = g_file_open_tmp ("squash-file-test-XXXXXX", &(data->filename[0]), &e);
  g_assert (e == NULL);
  data->fd[1] = g_file_open_tmp ("squash-file-test-XXXXXX", &(data->filename[1]), &e);
  g_assert (e == NULL);
  data->fd[2] = g_file_open_tmp ("squash-file-test-XXXXXX", &(data->filename[2]), &e);
  g_assert (e == NULL);
}

static void
triple_teardown (struct Triple* data, gconstpointer user_data) {
  unlink (data->filename[0]);
  unlink (data->filename[1]);
  unlink (data->filename[2]);
  close (data->fd[0]);
  close (data->fd[1]);
  close (data->fd[2]);
}

static void
test_file_splice (struct Triple* data, gconstpointer user_data) {
  const uint8_t offset_buf[4096] = { 0, };
  size_t offset;
  size_t bytes_written;
  int ires;

  FILE* uncompressed = fdopen (dup (data->fd[0]), "w+b");
  FILE* compressed = fdopen (dup (data->fd[1]), "w+b");
  FILE* decompressed = fdopen (dup (data->fd[2]), "w+b");

  bytes_written = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  g_assert_cmpint (bytes_written, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  /* Start in the middle of the file, just to make sure it works. */
  offset = (size_t) g_test_rand_int_range (1, 4096);
  bytes_written = fwrite (offset_buf, 1, offset, compressed);
  g_assert_cmpint (bytes_written, ==, offset);

  SquashStatus res = squash_splice (uncompressed, compressed, 0, SQUASH_STREAM_COMPRESS, (char*) user_data, NULL);
  g_assert (res == SQUASH_OK);

  {
    size_t compressed_length = squash_get_max_compressed_size ((char*) user_data, LOREM_IPSUM_LENGTH);
    uint8_t* compressed_data = malloc (compressed_length);
    res = squash_compress ((char*) user_data, &compressed_length, compressed_data, LOREM_IPSUM_LENGTH, LOREM_IPSUM, NULL);
    g_assert_cmpint (res, ==, SQUASH_OK);
    g_assert_cmpint (ftello (compressed), ==, compressed_length + offset);
    free (compressed_data);
  }

  ires = fseek (compressed, offset, SEEK_SET);
  g_assert_cmpint (ires, ==, 0);

  res = squash_splice (compressed, decompressed, 0, SQUASH_STREAM_DECOMPRESS, (char*) user_data, NULL);
  g_assert (res == SQUASH_OK);

  g_assert_cmpint (ftello (decompressed), ==, LOREM_IPSUM_LENGTH);

  fclose (uncompressed);
  fclose (compressed);
  fclose (decompressed);
}

static void
test_file_splice_partial (struct Triple* data, gconstpointer user_data) {
  uint8_t filler[LOREM_IPSUM_LENGTH] = { 0, };
  uint8_t decompressed_data[LOREM_IPSUM_LENGTH] = { 0, };
  size_t bytes;
  size_t len1, len2;

  FILE* uncompressed = fdopen (dup (data->fd[0]), "w+b");
  FILE* compressed = fdopen (dup (data->fd[1]), "w+b");
  FILE* decompressed = fdopen (dup (data->fd[2]), "w+b");

  for (len1 = 0 ; len1 < (size_t) LOREM_IPSUM_LENGTH ; len1++)
    filler[len1] = (uint8_t) g_test_rand_int_range (0x00, 0xff);

  bytes = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  g_assert_cmpint (bytes, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  len1 = (size_t) g_test_rand_int_range (128, LOREM_IPSUM_LENGTH - 1);
  len2 = (size_t) g_test_rand_int_range (64, len1 - 1);

  SquashStatus res = squash_splice (uncompressed, compressed, len1, SQUASH_STREAM_COMPRESS, (char*) user_data, NULL);
  g_assert_cmpint (res, ==, SQUASH_OK);
  rewind (uncompressed);
  rewind (compressed);

  res = squash_splice (compressed, decompressed, 0, SQUASH_STREAM_DECOMPRESS, (char*) user_data, NULL);
  g_assert (res == SQUASH_OK);
  g_assert_cmpint (ftello (decompressed), ==, (off_t) len1);
  rewind (compressed);
  rewind (decompressed);

  bytes = fread (decompressed_data, 1, LOREM_IPSUM_LENGTH, decompressed);
  g_assert (bytes == len1);
  g_assert (feof (decompressed));
  rewind (compressed);
  rewind (decompressed);

  memcpy (decompressed_data, filler, sizeof (decompressed_data));
  res = squash_splice (compressed, decompressed, len2, SQUASH_STREAM_DECOMPRESS, (char*) user_data, NULL);
  g_assert_cmpint (res, ==, SQUASH_OK);

  fclose (uncompressed);
  fclose (compressed);
  fclose (decompressed);
}

static gchar* squash_plugins_dir = NULL;

static GOptionEntry options[] = {
  { "squash-plugins", 0, 0, G_OPTION_ARG_STRING, &squash_plugins_dir, "Path to the Squash plugins directory", "DIR" },
  { NULL }
};

int
main (int argc, char** argv) {
  GError *error = NULL;
  GOptionContext *context;

  g_test_init (&argc, &argv, NULL);

  context = g_option_context_new ("- Squash unit test");
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_set_ignore_unknown_options (context, FALSE);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_error ("Option parsing failed: %s\n", error->message);
  }
  g_option_context_free (context);

  if (squash_plugins_dir != NULL) {
    g_setenv ("SQUASH_PLUGINS", squash_plugins_dir, TRUE);
  }

  g_test_add ("/file/io", struct Single, NULL, single_setup, test_file_io, single_teardown);
  g_test_add ("/file/splice/buffer", struct Triple, BUFFER_TEST_CODEC, triple_setup, test_file_splice, triple_teardown);
  g_test_add ("/file/splice/stream", struct Triple, STREAM_TEST_CODEC, triple_setup, test_file_splice, triple_teardown);
  g_test_add ("/file/splice/partial/buffer", struct Triple, BUFFER_TEST_CODEC, triple_setup, test_file_splice_partial, triple_teardown);
  g_test_add ("/file/splice/partial/stream", struct Triple, STREAM_TEST_CODEC, triple_setup, test_file_splice_partial, triple_teardown);

  return g_test_run ();
}
