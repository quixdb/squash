COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -Wno-attributes \
   -O3 \
  -std=c99 \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_SHARC

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/sharc
plugin_DATA = sharc.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-sharc.la

endif

SHARC_FILES = \
  sharc/src/byte_buffer.c \
  sharc/src/dictionary.c \
  sharc/src/direct_hash_transform.c \
  sharc/src/globals.c \
  sharc/src/hash_transform.c \
  sharc/src/header.c \
  sharc/src/sharc.c \
  sharc/src/sharc_transform.c \
  sharc/src/xor_hash_transform.c

libsquash@SQUASH_API_VERSION@_plugin_sharc_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_sharc_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_sharc_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_sharc_la_SOURCES = \
  squash-sharc.c \
  squash-sharc-stream.c \
  $(SHARC_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	sharc.md

-include $(top_srcdir)/git.mk
