COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_LZO

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzo
plugin_DATA = lzo.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzo.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzo_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzo_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzo_la_LIBADD = \
	$(COMMON_LIBS) \
  -llzo2

libsquash@SQUASH_API_VERSION@_plugin_lzo_la_SOURCES = squash-lzo.c

EXTRA_DIST = \
	$(plugin_DATA) \
	lzo.md

include $(top_srcdir)/git.mk
