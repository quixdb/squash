COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

LIBLZG_FILES = \
  liblzg/src/lib/checksum.c \
  liblzg/src/lib/decode.c \
  liblzg/src/lib/encode.c \
  liblzg/src/lib/version.c

if ENABLE_LZG

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzg
plugin_DATA = lzg.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzg.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzg_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzg_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzg_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_lzg_la_SOURCES = \
  squash-lzg.c \
  $(LIBLZG_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	lzg.md

-include $(top_srcdir)/git.mk
