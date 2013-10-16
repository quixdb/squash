COMMON_CFLAGS = \
   -O3 \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -Wno-missing-prototypes \
  -DSQUASH_IS_UNSTABLE

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_LZJB

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzjb
plugin_DATA = lzjb.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzjb.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzjb_la_CFLAGS = \
	$(COMMON_CFLAGS) \
  -Wno-error=missing-prototypes

libsquash@SQUASH_API_VERSION@_plugin_lzjb_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzjb_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_lzjb_la_SOURCES = \
  squash-lzjb.c \
  lzjb/lzjb.c \
  $(LZJB_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	lzjb.md

include $(top_srcdir)/git.mk
