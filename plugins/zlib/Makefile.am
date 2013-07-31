COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_ZLIB
plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/zlib
plugin_DATA = zlib.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-zlib.la
endif

libsquash@SQUASH_API_VERSION@_plugin_zlib_la_CFLAGS = \
	$(COMMON_CFLAGS) \
	$(ZLIB_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_zlib_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_zlib_la_LIBADD = \
	$(COMMON_LIBS) \
  $(ZLIB_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_zlib_la_SOURCES = squash-zlib.c

EXTRA_DIST = \
	$(plugin_DATA) \
	zlib.md

-include $(top_srcdir)/git.mk
