LIBS = \
  $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la \
  @GLIB_LIBS@

TESTS_ENVIRONMENT = \
  SQUASH_PLUGINS="$(top_srcdir)/plugins"

AM_CFLAGS = \
  -I$(top_srcdir) \
  @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  @GLIB_CFLAGS@

if ENABLE_UNIT_TESTING
TESTS = \
  buffer \
  random-data \
  stream-compress \
  stream-decompress \
  single-byte \
  threads
else
TESTS =
endif

noinst_PROGRAMS = $(TESTS)

buffer_SOURCES = test-codecs.c buffer.c
random_data_SOURCES = test-codecs.c random-data.c
stream_compress_SOURCES = test-codecs.c stream-compress.c
stream_decompress_SOURCES = test-codecs.c stream-decompress.c
single_byte_SOURCES = test-codecs.c single-byte.c
threads_SOURCES = test-codecs.c threads.c

CLEANFILES = \
  $(TESTS:=.log) \
  $(TESTS:=.trs) \
  test-suite.log

include $(top_srcdir)/git.mk
