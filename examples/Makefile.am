COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

noinst_PROGRAMS = \
  simple

simple_CFLAGS = $(COMMON_CFLAGS)
simple_LDADD = $(COMMON_LIBS)
simple_SOURCES = simple.c

include $(top_srcdir)/git.mk
