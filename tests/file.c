#include "test-codecs.h"

#include <unistd.h>

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
  g_free (data->filename);
}

static void
test_file_io (struct Single* data, gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  SquashFile* file = squash_file_open_codec (codec, data->filename, "w+", NULL);
  g_assert (file != NULL);

  SquashStatus res = squash_file_write (file, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  g_assert (res == SQUASH_OK);

  squash_file_close (file);

  file = squash_file_open_codec (codec, data->filename, "rb", NULL);
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
  g_free (data->filename[0]);
  g_free (data->filename[1]);
  g_free (data->filename[2]);
}

static void
test_file_splice (struct Triple* data, gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  const uint8_t offset_buf[4096] = { 0, };
  size_t offset;
  size_t bytes_written;
  int ires;

  ires = dup (data->fd[0]);
  g_assert (ires != -1);
  FILE* uncompressed = fdopen (ires, "w+b");
  ires = dup (data->fd[1]);
  g_assert (ires != -1);
  FILE* compressed = fdopen (ires, "w+b");
  ires = dup (data->fd[2]);
  g_assert (ires != -1);
  FILE* decompressed = fdopen (ires, "w+b");

  bytes_written = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  g_assert_cmpint (bytes_written, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  /* Start in the middle of the file, just to make sure it works. */
  offset = (size_t) g_test_rand_int_range (1, 4096);
  bytes_written = fwrite (offset_buf, 1, offset, compressed);
  g_assert_cmpint (bytes_written, ==, offset);

  g_assert_cmpint (ftello (compressed), ==, offset);

  SquashStatus res = squash_splice_codec (codec, SQUASH_STREAM_COMPRESS, compressed, uncompressed, 0, NULL);
  g_assert (res == SQUASH_OK);

  ires = fseek (compressed, offset, SEEK_SET);
  g_assert_cmpint (ires, ==, 0);

  res = squash_splice_codec (codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, 0, NULL);
  g_assert (res == SQUASH_OK);

  g_assert_cmpint (ftello (decompressed), ==, LOREM_IPSUM_LENGTH);

  rewind (decompressed);
  {
    uint8_t* decompressed_data = malloc (LOREM_IPSUM_LENGTH);
    g_assert (decompressed_data != NULL);

    size_t bytes_read = fread (decompressed_data, 1, LOREM_IPSUM_LENGTH, decompressed);
    g_assert_cmpint (bytes_read, ==, LOREM_IPSUM_LENGTH);

    g_assert (memcmp (decompressed_data, LOREM_IPSUM, LOREM_IPSUM_LENGTH) == 0);

    free (decompressed_data);
  }

  fclose (uncompressed);
  fclose (compressed);
  fclose (decompressed);
}

static void
test_file_splice_partial (struct Triple* data, gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  uint8_t filler[LOREM_IPSUM_LENGTH] = { 0, };
  uint8_t decompressed_data[LOREM_IPSUM_LENGTH] = { 0, };
  size_t bytes;
  size_t len1, len2;
  int ires;

  ires = dup (data->fd[0]);
  g_assert (ires != -1);
  FILE* uncompressed = fdopen (ires, "w+b");
  ires = dup (data->fd[1]);
  g_assert (ires != -1);
  FILE* compressed = fdopen (ires, "w+b");
  ires = dup (data->fd[2]);
  g_assert (ires != -1);
  FILE* decompressed = fdopen (ires, "w+b");

  for (len1 = 0 ; len1 < (size_t) LOREM_IPSUM_LENGTH ; len1++)
    filler[len1] = (uint8_t) g_test_rand_int_range (0x00, 0xff);

  bytes = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  g_assert_cmpint (bytes, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  len1 = (size_t) g_test_rand_int_range (128, LOREM_IPSUM_LENGTH - 1);
  len2 = (size_t) g_test_rand_int_range (64, len1 - 1);

  SquashStatus res = squash_splice_codec (codec, SQUASH_STREAM_COMPRESS, compressed, uncompressed, len1, NULL);
  g_assert_cmpint (res, ==, SQUASH_OK);
  g_assert_cmpint (ftello (uncompressed), ==, len1);
  rewind (uncompressed);
  rewind (compressed);

  res = squash_splice_codec (codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, 0, NULL);
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
  res = squash_splice_codec (codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, len2, NULL);
  g_assert_cmpint (res, ==, SQUASH_OK);

  fclose (uncompressed);
  fclose (compressed);
  fclose (decompressed);
}

static void
test_file_printf (struct Single* data, gconstpointer user_data) {
  SquashCodec* codec = (SquashCodec*) user_data;
  SquashFile* file = squash_file_open_codec (codec, data->filename, "w+", NULL);
  g_assert (file != NULL);

  SquashStatus res = squash_file_printf (file, "Hello, %s\n", "world");
  g_assert (res == SQUASH_OK);

  squash_file_close (file);

  file = squash_file_open_codec (codec, data->filename, "rb", NULL);
  g_assert (file != NULL);

  uint8_t decompressed[64];
  size_t total_read = 0;
  do {
    size_t bytes_read = 256;
    res = squash_file_read (file, &bytes_read, decompressed + total_read);
    assert (res > 0);
    total_read += bytes_read;
  } while (!squash_file_eof (file));

  static const size_t read_desired = 13; /* strlen ("Hello, world\n") */

  g_assert_cmpint (total_read, ==, read_desired);
  /* Note that we don't store the trailing null byte, so we need to
     add it back in here. */
  decompressed[read_desired] = 0x00;
  g_assert_cmpstr ((char*) decompressed, ==, "Hello, world\n");

  squash_file_close (file);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name;

  test_name =
    g_strdup_printf ("/file/printf/%s/%s",
                     squash_plugin_get_name (squash_codec_get_plugin (codec)),
                     squash_codec_get_name (codec));
  g_test_add (test_name, struct Single, codec, single_setup, test_file_printf, single_teardown);
  g_free (test_name);

  test_name =
    g_strdup_printf ("/file/io/%s/%s",
                     squash_plugin_get_name (squash_codec_get_plugin (codec)),
                     squash_codec_get_name (codec));
  g_test_add (test_name, struct Single, codec, single_setup, test_file_io, single_teardown);
  g_free (test_name);

  test_name =
    g_strdup_printf ("/file/splice/full/%s/%s",
                     squash_plugin_get_name (squash_codec_get_plugin (codec)),
                     squash_codec_get_name (codec));
  g_test_add (test_name, struct Triple, codec, triple_setup, test_file_splice, triple_teardown);
  g_free (test_name);

  test_name =
    g_strdup_printf ("/file/splice/partial/%s/%s",
                     squash_plugin_get_name (squash_codec_get_plugin (codec)),
                     squash_codec_get_name (codec));
  g_test_add (test_name, struct Triple, codec, triple_setup, test_file_splice_partial, triple_teardown);
  g_free (test_name);
}
