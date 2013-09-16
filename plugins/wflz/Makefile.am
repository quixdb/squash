COMMON_CFLAGS = \
  -O3 \
  -I$(top_srcdir) \
  @WARN_CFLAGS@ \
  -Wno-strict-prototypes \
  -Wno-missing-prototypes \
  -DSQUASH_IS_UNSTABLE

WFLZ_FILES = \
  wflz/wfLZ.c

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/wflz
plugin_DATA = wflz.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-wflz.la

libsquash@SQUASH_API_VERSION@_plugin_wflz_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_wflz_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_wflz_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_wflz_la_SOURCES = \
  $(WFLZ_FILES) \
  squash-wflz.c

EXTRA_DIST = \
	$(plugin_DATA) \
	wflz.md

include $(top_srcdir)/git.mk
