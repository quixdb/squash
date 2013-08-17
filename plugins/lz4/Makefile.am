COMMON_CFLAGS = \
   -O3 \
   -I$(top_srcdir) \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

LZ4_FILES = \
  lz4/lz4.c \
  lz4/lz4hc.c \
  lz4/xxhash.c

LZ4_CFLAGS = \
  -Wno-error=missing-prototypes \
  -Ilz4/

if ENABLE_LZ4

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lz4
plugin_DATA = lz4.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lz4.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lz4_la_CFLAGS = \
	$(COMMON_CFLAGS) \
  $(LZ4_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lz4_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lz4_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_lz4_la_SOURCES = \
  squash-lz4.c \
  $(LZ4_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	lz4.md

-include $(top_srcdir)/git.mk
