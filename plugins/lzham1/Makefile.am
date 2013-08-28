COMMON_CFLAGS = \
   -I$(top_srcdir) \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

LZHAM_FILES = \
  lzham/lzhamdecomp/lzham_checksum.cpp \
  lzham/lzhamdecomp/lzham_lzdecomp.cpp \
  lzham/lzhamdecomp/lzham_mem.cpp \
  lzham/lzhamdecomp/lzham_huffman_codes.cpp \
  lzham/lzhamdecomp/lzham_polar_codes.cpp \
  lzham/lzhamdecomp/lzham_prefix_coding.cpp \
  lzham/lzhamdecomp/lzham_symbol_codec.cpp \
  lzham/lzhamdecomp/lzham_assert.cpp \
  lzham/lzhamdecomp/lzham_timer.cpp \
  lzham/lzhamdecomp/lzham_platform.cpp \
  lzham/lzhamdecomp/lzham_vector.cpp \
  lzham/lzhamdecomp/lzham_lzdecompbase.cpp \
  lzham/lzhamcomp/lzham_pthreads_threading.cpp \
  lzham/lzhamcomp/lzham_lzbase.cpp \
  lzham/lzhamcomp/lzham_lzcomp.cpp \
  lzham/lzhamcomp/lzham_match_accel.cpp \
  lzham/lzhamcomp/lzham_lzcomp_state.cpp \
  lzham/lzhamcomp/lzham_lzcomp_internal.cpp

if ENABLE_LZHAM

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/lzham
plugin_DATA = lzham.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-lzham.la

endif

libsquash@SQUASH_API_VERSION@_plugin_lzham_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzham_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_lzham_la_LIBADD = \
	$(COMMON_LIBS) \
  -llzham

libsquash@SQUASH_API_VERSION@_plugin_lzham_la_SOURCES = squash-lzham.c

EXTRA_DIST = \
	$(plugin_DATA) \
	lzham.md

include $(top_srcdir)/git.mk
