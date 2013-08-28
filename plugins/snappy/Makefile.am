COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_SNAPPY

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/snappy
plugin_DATA = snappy.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-snappy.la

endif

libsquash@SQUASH_API_VERSION@_plugin_snappy_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_snappy_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_snappy_la_LIBADD = \
	$(COMMON_LIBS) \
  -lsnappy

libsquash@SQUASH_API_VERSION@_plugin_snappy_la_SOURCES = squash-snappy.c

EXTRA_DIST = \
	$(plugin_DATA) \
	snappy.md

include $(top_srcdir)/git.mk
