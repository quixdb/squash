#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "test-squash.h"

#include "../squash/tinycthread/source/tinycthread.h"

static unsigned int
squash_test_cpu_count (void) {
  static unsigned int count = 0;

  if (SQUASH_UNLIKELY (count == 0)) {
    unsigned int c = 0;

#if defined(_WIN32)
    /* TODO: Figure out how Windows programmers ever get anything done. */
    PDWORD lpProcessAffinityMask;
    PDWORD lpSystemAffinityMask;

    if (!GetProcessAffinityMask (GetCurrentProcess (), &lpProcessAffinityMask, &lpSystemAffinityMask)) {
      DWORD ec = GetLastError ();
      count = 1;
    } else {
      for (unsigned int i = 0 ; lpProcessAffinityMask != 0 ; lpProcessAffinityMask >>= 1) {
        if (lpProcessAffinityMask & 1)
          c++;
      }
    }
#elif defined(_SC_NPROCESSORS_ONLN)
    c = (unsigned int) sysconf (_SC_NPROCESSORS_ONLN);
#else
#warning No idea how to get the number of CPUs
#endif

    count = (c == 0) ? 1 : c;
  }

  return count;
}

static int
compress_buffer_thread_func (SquashCodec* codec) {
  const size_t max_compressed_length = squash_codec_get_max_compressed_size (codec, LOREM_IPSUM_LENGTH);
  size_t compressed_length;
  size_t decompressed_length = LOREM_IPSUM_LENGTH;
  uint8_t* compressed = munit_malloc (max_compressed_length);
  uint8_t* decompressed = munit_malloc (LOREM_IPSUM_LENGTH);
  SquashStatus res;
  int i = 0;

  for ( ; i < 8 ; i++ ) {
    compressed_length = max_compressed_length;

    res = squash_codec_compress (codec, &compressed_length, compressed, LOREM_IPSUM_LENGTH, (uint8_t*) LOREM_IPSUM, NULL);
    SQUASH_ASSERT_OK(res);

    res = squash_codec_decompress (codec, &decompressed_length, decompressed, compressed_length, compressed, NULL);
    SQUASH_ASSERT_OK(res);
    munit_assert_cmp_size (decompressed_length, ==, LOREM_IPSUM_LENGTH);

    munit_assert_memory_equal(LOREM_IPSUM_LENGTH, decompressed, LOREM_IPSUM);
  }

  free (compressed);
  free (decompressed);

  return (int) MUNIT_OK;
}


static MunitResult
squash_test_threads_buffer(MUNIT_UNUSED const MunitParameter params[], void* user_data) {
  munit_assert_non_null(user_data);
  SquashCodec* codec = (SquashCodec*) user_data;

  const unsigned int n_threads = squash_test_cpu_count ();
  thrd_t* threads = munit_newa(thrd_t, n_threads);

  for (unsigned int i = 0 ; i < n_threads ; i++) {
    const int r = thrd_create(&(threads[i]), (thrd_start_t) compress_buffer_thread_func, codec);
    munit_assert_cmp_int (r, ==, thrd_success);
  }

  for (unsigned int i = 0 ; i < n_threads ; i++) {
    int retval;
    const int r = thrd_join(threads[i], &retval);
    munit_assert_cmp_int (r, ==, thrd_success);
    if (MUNIT_UNLIKELY(retval != MUNIT_OK)) {
      free (threads);
      return (MunitResult) retval;
    }
  }

  free (threads);

  return MUNIT_OK;
}

MunitTest squash_threads_tests[] = {
  { (char*) "/buffer", squash_test_threads_buffer, squash_test_get_codec, NULL, MUNIT_TEST_OPTION_NONE, SQUASH_CODEC_PARAMETER },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

MunitSuite squash_test_suite_threads = {
  (char*) "/threads",
  squash_threads_tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};
