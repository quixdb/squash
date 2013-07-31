COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_LZF

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzf
plugin_DATA = lzf.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzf.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzf_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzf_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzf_la_LIBADD = \
	$(COMMON_LIBS) \
  -llzf

libsquash@SQUASH_API_VERSION@_plugin_lzf_la_SOURCES = squash-lzf.c

EXTRA_DIST = \
	$(plugin_DATA) \
	lzf.md

-include $(top_srcdir)/git.mk
