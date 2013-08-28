COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -Wno-pointer-to-int-cast \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

LZMAT_FILES = \
  lzmat/lzmat_dec.c \
  lzmat/lzmat_enc.c

if ENABLE_LZMAT

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzmat
plugin_DATA = lzmat.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzmat.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzmat_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzmat_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzmat_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_lzmat_la_SOURCES = \
  squash-lzmat.c \
  $(LZMAT_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	lzmat.md

include $(top_srcdir)/git.mk
