if ENABLE_ZPAQ
plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/zpaq
plugin_DATA = zpaq.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-zpaq.la
endif

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_CPPFLAGS = \
   -I$(top_srcdir) \
  -DSQUASH_IS_UNSTABLE \
  @ZPAQ_SSE2_CPPFLAGS@

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_LIBADD = \
  $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_SOURCES = \
  libzpaq.cpp \
  divsufsort.c \
  divsufsort.h \
  squash-zpaq.cpp

EXTRA_DIST = \
	$(plugin_DATA) \
	zpaq.md

-include $(top_srcdir)/git.mk
