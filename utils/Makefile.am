COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

bin_PROGRAMS = \
  squash

squash_CFLAGS = $(COMMON_CFLAGS)
squash_LDADD = $(COMMON_LIBS)

squash_SOURCES = \
  squash.c

include $(top_srcdir)/git.mk
