#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 200112L)
#  undef _POSIX_C_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200112L
#endif

#include "test-squash.h"

#ifdef _MSC_VER
#  define off_t long
#  define ftello ftell
#else
#  include <unistd.h>
#endif

struct Single {
  SquashCodec* codec;
  FILE* file;
};

struct Triple {
  SquashCodec* codec;
  FILE* file[3];
};

static void*
squash_test_single_setup(const MunitParameter params[], MUNIT_UNUSED void* user_data) {
  struct Single* data = munit_new(struct Single);
  data->codec = squash_get_codec (munit_parameters_get(params, "codec"));
  munit_assert_not_null (data);

  data->file = tmpfile();
  munit_assert_not_null (data->file);

  return data;
}

static void
squash_test_single_tear_down(void* user_data) {
  struct Single* data = (struct Single*) user_data;
  munit_assert_not_null (data);

  fclose (data->file);

  free (data);
}

static void*
squash_test_triple_setup(const MunitParameter params[], MUNIT_UNUSED void* user_data) {
  struct Triple* data = munit_new(struct Triple);
  data->codec = squash_get_codec (munit_parameters_get(params, "codec"));
  munit_assert_not_null (data);

  for (unsigned int n = 0 ; n < 3 ; n++) {
    data->file[n] = tmpfile();
    munit_assert_not_null (data->file[n]);
  }

  return data;
}

static void
squash_test_triple_tear_down(void* user_data) {
  struct Triple* data = (struct Triple*) user_data;
  munit_assert_not_null (data);

  for (unsigned int n = 0 ; n < 3 ; n++)
    fclose (data->file[n]);

  free (data);
}

static MunitResult
squash_test_io(const MunitParameter params[], void* user_data) {
  struct Single* data = (struct Single*) user_data;
  munit_assert_not_null (data);

  SquashFile* file = squash_file_steal (data->codec, data->file, NULL);
  munit_assert_not_null (file);
  SquashStatus res = squash_file_write (file, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  SQUASH_ASSERT_OK(res);
  squash_file_free (file, NULL);

  fflush (data->file);
  rewind (data->file);

  file = squash_file_steal (data->codec, data->file, NULL);
  uint8_t decompressed[LOREM_IPSUM_LENGTH];
  size_t total_read = 0;
  do {
    size_t bytes_read = (size_t) munit_rand_int_range (32, 256);
    res = squash_file_read (file, &bytes_read, decompressed + total_read);
    SQUASH_ASSERT_NO_ERROR(res);
    total_read += bytes_read;
    munit_assert_size (total_read, <=, LOREM_IPSUM_LENGTH);
  } while (!squash_file_eof (file));

  munit_assert_size (total_read, ==, LOREM_IPSUM_LENGTH);
  munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);

  squash_file_free (file, NULL);

  return MUNIT_OK;
}

static MunitResult
squash_test_splice_full(const MunitParameter params[], void* user_data) {
  struct Triple* data = (struct Triple*) user_data;
  munit_assert_not_null (data);
  const uint8_t offset_buf[4096] = { 0, };
  size_t offset;
  size_t bytes_written;
  int ires;

  FILE* uncompressed = data->file[0];
  FILE* compressed   = data->file[1];
  FILE* decompressed = data->file[2];

  bytes_written = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  munit_assert_size (bytes_written, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  /* Start in the middle of the file, just to make sure it works. */
  offset = (size_t) munit_rand_int_range (1, sizeof(offset_buf));
  bytes_written = fwrite (offset_buf, 1, offset, compressed);
  munit_assert_size (bytes_written, ==, offset);
  munit_assert_int (ftello (compressed), ==, offset);

  SquashStatus res = squash_splice (data->codec, SQUASH_STREAM_COMPRESS, compressed, uncompressed, 0, NULL);
  SQUASH_ASSERT_OK(res);

  ires = fseek (compressed, offset, SEEK_SET);
  munit_assert_int (ires, ==, 0);

  res = squash_splice (data->codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, 0, NULL);
  SQUASH_ASSERT_OK(res);

  munit_assert_int (ftello (decompressed), ==, LOREM_IPSUM_LENGTH);

  rewind (decompressed);
  {
    uint8_t* decompressed_data = munit_malloc (LOREM_IPSUM_LENGTH);

    size_t bytes_read = fread (decompressed_data, 1, LOREM_IPSUM_LENGTH, decompressed);
    munit_assert_int (bytes_read, ==, LOREM_IPSUM_LENGTH);

    munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed_data, LOREM_IPSUM);

    free (decompressed_data);
  }

  return MUNIT_OK;
}

static MunitResult
squash_test_splice_partial(const MunitParameter params[], void* user_data) {
  struct Triple* data = (struct Triple*) user_data;
  SquashCodec* codec = data->codec;
  uint8_t filler[LOREM_IPSUM_LENGTH] = { 0, };
  uint8_t decompressed_data[LOREM_IPSUM_LENGTH] = { 0, };
  size_t bytes;
  size_t len1, len2;

  FILE* uncompressed = data->file[0];
  FILE* compressed   = data->file[1];
  FILE* decompressed = data->file[2];

  for (len1 = 0 ; len1 < (size_t) LOREM_IPSUM_LENGTH ; len1++)
    filler[len1] = (uint8_t) munit_rand_int_range (0x00, 0xff);

  bytes = fwrite (LOREM_IPSUM, 1, LOREM_IPSUM_LENGTH, uncompressed);
  munit_assert_size (bytes, ==, LOREM_IPSUM_LENGTH);
  fflush (uncompressed);
  rewind (uncompressed);

  len1 = (size_t) munit_rand_int_range (128, LOREM_IPSUM_LENGTH - 1);
  len2 = (size_t) munit_rand_int_range (64, len1 - 1);

  SquashStatus res = squash_splice (codec, SQUASH_STREAM_COMPRESS, compressed, uncompressed, len1, NULL);
  munit_assert_size (res, ==, SQUASH_OK);
  munit_assert_size (ftello (uncompressed), ==, len1);
  rewind (uncompressed);
  rewind (compressed);

  res = squash_splice (codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, 0, NULL);
  SQUASH_ASSERT_OK (res);
  munit_assert_size (ftello (decompressed), ==, (off_t) len1);
  rewind (compressed);
  rewind (decompressed);

  bytes = fread (decompressed_data, 1, LOREM_IPSUM_LENGTH, decompressed);
  munit_assert_size (bytes, ==, len1);
  munit_assert (feof (decompressed));
  rewind (compressed);
  rewind (decompressed);

  memcpy (decompressed_data, filler, sizeof (decompressed_data));
  res = squash_splice (codec, SQUASH_STREAM_DECOMPRESS, decompressed, compressed, len2, NULL);
  SQUASH_ASSERT_OK(res);

  return MUNIT_OK;
}

#define HELLO_WORLD_LENGTH ((size_t) 13)

static MunitResult
squash_test_printf(const MunitParameter params[], void* user_data) {
  struct Single* data = (struct Single*) user_data;
  munit_assert_not_null (data);

  SquashCodec* codec = data->codec;
  SquashFile* file = squash_file_steal (codec, data->file, NULL);
  munit_assert_not_null (file);
  uint8_t decompressed[LOREM_IPSUM_LENGTH + HELLO_WORLD_LENGTH + 1];

  SquashStatus res = squash_file_printf (file, "Hello, %s\n", "world");
  SQUASH_ASSERT_STATUS(res, SQUASH_OK);
  res = squash_file_printf (file, (const char*) LOREM_IPSUM);
  SQUASH_ASSERT_STATUS(res, SQUASH_OK);

  squash_file_free (file, NULL);
  rewind (data->file);

  file = squash_file_steal (codec, data->file, NULL);
  munit_assert_not_null (file);

  size_t total_read = 0;
  do {
    size_t bytes_read = 256;
    res = squash_file_read (file, &bytes_read, decompressed + total_read);
    assert (res > 0);
    total_read += bytes_read;
  } while (!squash_file_eof (file));

  munit_assert_size (total_read, ==, HELLO_WORLD_LENGTH + LOREM_IPSUM_LENGTH);
  /* Note that we don't store the trailing null byte, so we need to
     add it back in here. */
  decompressed[HELLO_WORLD_LENGTH + LOREM_IPSUM_LENGTH] = 0x00;
  munit_assert_memory_equal (HELLO_WORLD_LENGTH, decompressed, "Hello, world\n");
  munit_assert_string_equal ((char*) decompressed + HELLO_WORLD_LENGTH, (char*) LOREM_IPSUM);

  squash_file_free (file, NULL);

  return MUNIT_OK;
}

MunitTest squash_file_tests[] = {
  { (char*) "/io", squash_test_io, squash_test_single_setup, squash_test_single_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/splice/full", squash_test_splice_full, squash_test_triple_setup, squash_test_triple_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/splice/partial", squash_test_splice_partial, squash_test_triple_setup, squash_test_triple_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { (char*) "/printf", squash_test_printf, squash_test_single_setup, squash_test_single_tear_down, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_file = {
  (char*) "/file",
  squash_file_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
