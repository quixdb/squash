COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_BZIP2

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/bzip2
plugin_DATA = bzip2.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-bzip2.la

endif

libsquash@SQUASH_API_VERSION@_plugin_bzip2_la_CFLAGS = $(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_bzip2_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_bzip2_la_LIBADD = \
	$(COMMON_LIBS) \
  -lbz2

libsquash@SQUASH_API_VERSION@_plugin_bzip2_la_SOURCES = squash-bzip2.c

EXTRA_DIST = \
	$(plugin_DATA) \
	bzip2.md

-include $(top_srcdir)/git.mk
