COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_LZMA

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzma
plugin_DATA = lzma.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzma.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzma_la_CFLAGS = \
  $(COMMON_CFLAGS) \
  $(LZMA_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzma_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzma_la_LIBADD = \
	$(COMMON_LIBS) \
  $(LZMA_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_lzma_la_SOURCES = \
  squash-lzma.c

EXTRA_DIST = \
	$(plugin_DATA) \
	lzma.md

include $(top_srcdir)/git.mk
