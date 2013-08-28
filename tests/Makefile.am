COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

TESTS_ENVIRONMENT = \
  SQUASH_PLUGINS="$(top_srcdir)/plugins"

TESTS_CFLAGS = \
  $(COMMON_CFLAGS) \
  @GLIB_CFLAGS@

TESTS_LIBS = \
  $(COMMON_LIBS) \
  @GLIB_LIBS@

TESTS = \
  check-plugins

if HAVE_GLIB
noinst_PROGRAMS = $(TESTS)
endif

check_plugins_CFLAGS = $(TESTS_CFLAGS)
check_plugins_LDADD = $(TESTS_LIBS)
check_plugins_SOURCES = check-plugins.c

CLEANFILES = \
  $(TESTS:=.log) \
  $(TESTS:=.trs) \
  test-suite.log

include $(top_srcdir)/git.mk
