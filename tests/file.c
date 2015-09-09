#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_26
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_26

#include <glib.h>
#include <squash/squash.h>

#include <unistd.h>
#include <string.h>

#define TEST_CODEC "zlib:gzip"

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
  SquashFile* file = squash_file_open (data->filename, "w+", TEST_CODEC, NULL);
  g_assert (file != NULL);

  SquashStatus res = squash_file_write (file, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM);
  assert (res == SQUASH_OK);

  squash_file_close (file);

  file = squash_file_open (data->filename, "r", TEST_CODEC, NULL);
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

int
main (int argc, char** argv) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/file/io", struct Single, NULL, single_setup, test_file_io, single_teardown);

  return g_test_run ();
}
